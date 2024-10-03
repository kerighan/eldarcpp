#pragma once

#include <string>
#include <memory>
#include <vector>
#include <regex>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iostream>

class QueryNode {
public:
    virtual ~QueryNode() = default;
    virtual std::string toString() const = 0;
    virtual std::string toStringFlattenedORs() const { return toString(); }
};

class WordNode : public QueryNode {
private:
    std::string word;
public:
    WordNode(const std::string& w) : word(w) {}
    std::string toString() const override { return word; }
    std::string toStringFlattenedORs() const override { return word; }
    const std::string& getWord() const { return word; }  // Add this line
};

class NotNode : public QueryNode {
private:
    std::unique_ptr<QueryNode> child;
public:
    NotNode(std::unique_ptr<QueryNode> c) : child(std::move(c)) {}
    std::string toString() const override { return "NOT " + child->toString(); }
    std::string toStringFlattenedORs() const override { return "NOT " + child->toStringFlattenedORs(); }
    const QueryNode* getChild() const { return child.get(); }
};

class BinaryOpNode : public QueryNode {
protected:
    std::unique_ptr<QueryNode> left;
    std::unique_ptr<QueryNode> right;
    std::string op;
public:
    BinaryOpNode(std::unique_ptr<QueryNode> l, std::unique_ptr<QueryNode> r, const std::string& o)
        : left(std::move(l)), right(std::move(r)), op(o) {}
    std::string toString() const override {
        return "(" + left->toString() + " " + op + " " + right->toString() + ")";
    }
    const QueryNode* getLeft() const { return left.get(); }
    const QueryNode* getRight() const { return right.get(); }
};

class AndNode : public BinaryOpNode {
public:
    AndNode(std::unique_ptr<QueryNode> l, std::unique_ptr<QueryNode> r)
        : BinaryOpNode(std::move(l), std::move(r), "AND") {}
    std::string toStringFlattenedORs() const override {
        return "(" + left->toStringFlattenedORs() + " AND " + right->toStringFlattenedORs() + ")";
    }
};

class OrNode : public BinaryOpNode {
public:
    OrNode(std::unique_ptr<QueryNode> l, std::unique_ptr<QueryNode> r)
        : BinaryOpNode(std::move(l), std::move(r), "OR") {}
    std::string toStringFlattenedORs() const override {
        std::vector<std::string> terms;
        flattenOR(terms);
        return "(" + join(terms, " OR ") + ")";
    }

private:
    void flattenOR(std::vector<std::string>& terms) const {
        if (auto orLeft = dynamic_cast<const OrNode*>(left.get())) {
            orLeft->flattenOR(terms);
        } else {
            terms.push_back(left->toStringFlattenedORs());
        }
        if (auto orRight = dynamic_cast<const OrNode*>(right.get())) {
            orRight->flattenOR(terms);
        } else {
            terms.push_back(right->toStringFlattenedORs());
        }
    }

    static std::string join(const std::vector<std::string>& v, const std::string& delim) {
        std::string result;
        for (size_t i = 0; i < v.size(); ++i) {
            if (i > 0) result += delim;
            result += v[i];
        }
        return result;
    }
};

class AndNotNode : public BinaryOpNode {
public:
    AndNotNode(std::unique_ptr<QueryNode> l, std::unique_ptr<QueryNode> r)
        : BinaryOpNode(std::move(l), std::move(r), "AND NOT") {}
    std::string toStringFlattenedORs() const override {
        return "(" + left->toStringFlattenedORs() + " AND NOT " + right->toStringFlattenedORs() + ")";
    }
};

class QueryTree {
private:
    std::unique_ptr<QueryNode> root;

    static std::string strip_brackets(const std::string& s) {
        if (s.front() == '(' && s.back() == ')' && is_balanced(s.substr(1, s.length() - 2))) {
            return s.substr(1, s.length() - 2);
        }
        return s;
    }

    static bool is_balanced(const std::string& s) {
        int count = 0;
        for (char c : s) {
            if (c == '(') count++;
            if (c == ')') count--;
            if (count < 0) return false;
        }
        return count == 0;
    }

    static std::unique_ptr<QueryNode> parse_query(const std::string& query, bool ignore_case = true) {
        std::string q = strip_brackets(query);

        if (q.empty()) {
            throw std::runtime_error("Empty query");
        }

        if (q.front() == '"' && q.back() == '"' && std::count(q.begin(), q.end(), '"') == 2) {
            if (ignore_case) {
                std::transform(q.begin(), q.end(), q.begin(), ::tolower);
            }
            return std::make_unique<WordNode>(q.substr(1, q.length() - 2));
        }

        if (q.substr(0, 4) == "NOT " || q.substr(0, 4) == "not ") {
            return std::make_unique<NotNode>(parse_query(q.substr(4), ignore_case));
        }

        std::regex op_regex(R"(\s+(AND NOT|AND|OR)\s+)");
        std::sregex_iterator it(q.begin(), q.end(), op_regex);
        std::sregex_iterator end;

        std::string left_part = q;
        std::string op;
        std::string right_part;

        for (; it != end; ++it) {
            std::smatch match = *it;
            left_part = q.substr(0, match.position());
            op = match.str();
            right_part = q.substr(match.position() + match.length());

            if (is_balanced(left_part) && is_balanced(right_part)) {
                break;
            }
        }

        if (op.empty()) {
            if (ignore_case) {
                std::transform(q.begin(), q.end(), q.begin(), ::tolower);
            }
            return std::make_unique<WordNode>(q);
        }

        op = op.substr(1, op.length() - 2);
        std::transform(op.begin(), op.end(), op.begin(), ::tolower);

        if (op == "or") {
            return std::make_unique<OrNode>(parse_query(left_part, ignore_case), parse_query(right_part, ignore_case));
        } else if (op == "and") {
            return std::make_unique<AndNode>(parse_query(left_part, ignore_case), parse_query(right_part, ignore_case));
        } else if (op == "and not") {
            return std::make_unique<AndNotNode>(parse_query(left_part, ignore_case), parse_query(right_part, ignore_case));
        }

        throw std::runtime_error("Invalid operator");
    }

    static std::unique_ptr<QueryNode> cloneNode(const QueryNode* node) {
        if (const auto* wordNode = dynamic_cast<const WordNode*>(node)) {
            return std::make_unique<WordNode>(wordNode->getWord());
        } else if (const auto* notNode = dynamic_cast<const NotNode*>(node)) {
            return std::make_unique<NotNode>(cloneNode(notNode->getChild()));
        } else if (const auto* andNode = dynamic_cast<const AndNode*>(node)) {
            return std::make_unique<AndNode>(cloneNode(andNode->getLeft()), cloneNode(andNode->getRight()));
        } else if (const auto* orNode = dynamic_cast<const OrNode*>(node)) {
            return std::make_unique<OrNode>(cloneNode(orNode->getLeft()), cloneNode(orNode->getRight()));
        } else if (const auto* andNotNode = dynamic_cast<const AndNotNode*>(node)) {
            return std::make_unique<AndNotNode>(cloneNode(andNotNode->getLeft()), cloneNode(andNotNode->getRight()));
        }
        throw std::runtime_error("Unknown node type");
    }

    static std::unique_ptr<QueryNode> expandNodeAt(
        const QueryNode* node,
        const std::vector<int>& path,
        size_t pathIndex,
        const std::string& newWord,
        const std::string& op
    ) {
        if (pathIndex == path.size()) {
            if (op == "AND") {
                return std::make_unique<AndNode>(
                    cloneNode(node),
                    std::make_unique<WordNode>(newWord)
                );
            } else if (op == "OR") {
                return std::make_unique<OrNode>(
                    cloneNode(node),
                    std::make_unique<WordNode>(newWord)
                );
            } else if (op == "AND NOT") {
                return std::make_unique<AndNotNode>(
                    cloneNode(node),
                    std::make_unique<WordNode>(newWord)
                );
            } else {
                throw std::runtime_error("Invalid operator for expansion");
            }
        }

        if (const auto* binaryNode = dynamic_cast<const BinaryOpNode*>(node)) {
            auto newLeft = (path[pathIndex] == 0) 
                ? expandNodeAt(binaryNode->getLeft(), path, pathIndex + 1, newWord, op)
                : cloneNode(binaryNode->getLeft());
            auto newRight = (path[pathIndex] == 1)
                ? expandNodeAt(binaryNode->getRight(), path, pathIndex + 1, newWord, op)
                : cloneNode(binaryNode->getRight());

            if (dynamic_cast<const AndNode*>(binaryNode)) {
                return std::make_unique<AndNode>(std::move(newLeft), std::move(newRight));
            } else if (dynamic_cast<const OrNode*>(binaryNode)) {
                return std::make_unique<OrNode>(std::move(newLeft), std::move(newRight));
            } else if (dynamic_cast<const AndNotNode*>(binaryNode)) {
                return std::make_unique<AndNotNode>(std::move(newLeft), std::move(newRight));
            }
        } else if (const auto* notNode = dynamic_cast<const NotNode*>(node)) {
            return std::make_unique<NotNode>(
                expandNodeAt(notNode->getChild(), path, pathIndex + 1, newWord, op)
            );
        }

        // We shouldn't reach here if the path is valid
        throw std::runtime_error("Invalid path for expansion");
    }

    void generateExpansionsRecursive(const QueryNode* node, const std::string& newWord, 
                                     std::vector<int>& currentPath,
                                     std::vector<std::tuple<std::unique_ptr<QueryNode>, std::vector<int>, std::string>>& expansions) const {
        // Expand at the current node only if it's a binary node or the root
        if (currentPath.empty() || dynamic_cast<const BinaryOpNode*>(node)) {
            for (const auto& op : {"AND", "OR", "AND NOT"}) {
                expansions.emplace_back(expandNodeAt(root.get(), currentPath, 0, newWord, op), currentPath, op);
            }
        }

        // Recursively traverse children
        if (const auto* binaryNode = dynamic_cast<const BinaryOpNode*>(node)) {
            currentPath.push_back(0);
            generateExpansionsRecursive(binaryNode->getLeft(), newWord, currentPath, expansions);
            currentPath.back() = 1;
            generateExpansionsRecursive(binaryNode->getRight(), newWord, currentPath, expansions);
            currentPath.pop_back();
        } else if (const auto* notNode = dynamic_cast<const NotNode*>(node)) {
            currentPath.push_back(0);
            generateExpansionsRecursive(notNode->getChild(), newWord, currentPath, expansions);
            currentPath.pop_back();
        }
    }
    
    void collectWordNodePaths(const QueryNode* node, std::vector<int>& currentPath, std::vector<std::vector<int>>& paths) const {
        if (dynamic_cast<const WordNode*>(node)) {
            paths.push_back(currentPath);
        } else if (const auto* binaryNode = dynamic_cast<const BinaryOpNode*>(node)) {
            currentPath.push_back(0);
            collectWordNodePaths(binaryNode->getLeft(), currentPath, paths);
            currentPath.pop_back();

            currentPath.push_back(1);
            collectWordNodePaths(binaryNode->getRight(), currentPath, paths);
            currentPath.pop_back();
        } else if (const auto* notNode = dynamic_cast<const NotNode*>(node)) {
            currentPath.push_back(0);
            collectWordNodePaths(notNode->getChild(), currentPath, paths);
            currentPath.pop_back();
        }
    }

    std::unique_ptr<QueryNode> cloneAndReplaceAtPath(const QueryNode* node, const std::vector<int>& path, size_t pathIndex, const std::string& newWord, const std::string& op) const {
        if (pathIndex == path.size()) {
            if (const auto* wordNode = dynamic_cast<const WordNode*>(node)) {
                // Replace the WordNode with a BinaryOpNode
                auto leftNode = cloneNode(wordNode);
                auto rightNode = std::make_unique<WordNode>(newWord);
                if (op == "AND") {
                    return std::make_unique<AndNode>(std::move(leftNode), std::move(rightNode));
                } else if (op == "OR") {
                    return std::make_unique<OrNode>(std::move(leftNode), std::move(rightNode));
                } else if (op == "AND NOT") {
                    return std::make_unique<AndNotNode>(std::move(leftNode), std::move(rightNode));
                } else {
                    throw std::runtime_error("Invalid operator");
                }
            } else {
                throw std::runtime_error("Expected WordNode at the end of path");
            }
        } else {
            if (const auto* binaryNode = dynamic_cast<const BinaryOpNode*>(node)) {
                std::unique_ptr<QueryNode> newLeft, newRight;
                if (path[pathIndex] == 0) {
                    newLeft = cloneAndReplaceAtPath(binaryNode->getLeft(), path, pathIndex + 1, newWord, op);
                    newRight = cloneNode(binaryNode->getRight());
                } else {
                    newLeft = cloneNode(binaryNode->getLeft());
                    newRight = cloneAndReplaceAtPath(binaryNode->getRight(), path, pathIndex + 1, newWord, op);
                }
                if (dynamic_cast<const AndNode*>(binaryNode)) {
                    return std::make_unique<AndNode>(std::move(newLeft), std::move(newRight));
                } else if (dynamic_cast<const OrNode*>(binaryNode)) {
                    return std::make_unique<OrNode>(std::move(newLeft), std::move(newRight));
                } else if (dynamic_cast<const AndNotNode*>(binaryNode)) {
                    return std::make_unique<AndNotNode>(std::move(newLeft), std::move(newRight));
                } else {
                    throw std::runtime_error("Unknown binary node type");
                }
            } else if (const auto* notNode = dynamic_cast<const NotNode*>(node)) {
                auto newChild = cloneAndReplaceAtPath(notNode->getChild(), path, pathIndex + 1, newWord, op);
                return std::make_unique<NotNode>(std::move(newChild));
            } else {
                throw std::runtime_error("Unexpected node type in path");
            }
        }
    }

public:
    QueryTree(const std::string& query, bool ignore_case = true) {
        root = parse_query(query, ignore_case);
    }

    // Copy constructor
    QueryTree(const QueryTree& other) : root(cloneNode(other.root.get())) {}

    std::string toString(bool flattened = true) const {
        if (flattened) {
            return toStringFlattenedORs();
        }
        return root ? root->toString() : "";
    }

    std::string toStringFlattenedORs() const {
        return root ? root->toStringFlattenedORs() : "";
    }

    void expand(const std::vector<int>& path, const std::string& newWord, const std::string& op = "AND") {
        root = expandNodeAt(root.get(), path, 0, newWord, op);
    }

    std::vector<QueryTree> generateAllExpansions(const std::string& newWord) const {
        // Collect paths to all WordNodes
        std::vector<std::vector<int>> wordNodePaths;
        std::vector<int> currentPath;
        collectWordNodePaths(root.get(), currentPath, wordNodePaths);

        std::vector<QueryTree> expansions;
        for (const auto& path : wordNodePaths) {
            for (const auto& op : {"AND", "OR", "AND NOT"}) {
                // Clone the tree and replace the WordNode at path
                auto newRoot = cloneAndReplaceAtPath(root.get(), path, 0, newWord, op);
                QueryTree newTree(*this);
                newTree.root = std::move(newRoot);
                expansions.push_back(std::move(newTree));
            }
        }
        return expansions;
    }

    const QueryNode* getRoot() const { return root.get(); }

    std::string repr() const {
        return "QueryTree(\"" + toString() + "\")";
    }
};
