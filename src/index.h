#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>
#include <set>
#include <fstream>
#include "query_tree.h"

class Index {
private:
    std::unordered_map<std::string, std::vector<int>> inverted_index;
    int current_doc_id;

    std::vector<int> evaluate_node(const QueryNode* node) const {
        if (const auto* word_node = dynamic_cast<const WordNode*>(node)) {
            return get_postings(word_node->getWord());
        } else if (const auto* not_node = dynamic_cast<const NotNode*>(node)) {
            return evaluate_not(not_node);
        } else if (const auto* and_node = dynamic_cast<const AndNode*>(node)) {
            return evaluate_and(and_node);
        } else if (const auto* or_node = dynamic_cast<const OrNode*>(node)) {
            return evaluate_or(or_node);
        } else if (const auto* andnot_node = dynamic_cast<const AndNotNode*>(node)) {
            return evaluate_andnot(andnot_node);
        }
        return {};
    }

    std::vector<int> evaluate_not(const NotNode* node) const {
        std::vector<int> child_result = evaluate_node(node->getChild());
        std::set<int> child_set(child_result.begin(), child_result.end());
        std::vector<int> result;
        for (int i = 0; i < current_doc_id; ++i) {
            if (child_set.find(i) == child_set.end()) {
                result.push_back(i);
            }
        }
        return result;
    }

    std::vector<int> evaluate_and(const AndNode* node) const {
        std::vector<int> left_result = evaluate_node(node->getLeft());
        std::vector<int> right_result = evaluate_node(node->getRight());
        std::vector<int> result;
        std::set_intersection(left_result.begin(), left_result.end(),
                              right_result.begin(), right_result.end(),
                              std::back_inserter(result));
        return result;
    }

    std::vector<int> evaluate_or(const OrNode* node) const {
        std::vector<int> left_result = evaluate_node(node->getLeft());
        std::vector<int> right_result = evaluate_node(node->getRight());
        std::vector<int> result;
        std::set_union(left_result.begin(), left_result.end(),
                       right_result.begin(), right_result.end(),
                       std::back_inserter(result));
        return result;
    }

    std::vector<int> evaluate_andnot(const AndNotNode* node) const {
        std::vector<int> left_result = evaluate_node(node->getLeft());
        std::vector<int> right_result = evaluate_node(node->getRight());
        std::vector<int> result;
        std::set_difference(left_result.begin(), left_result.end(),
                            right_result.begin(), right_result.end(),
                            std::back_inserter(result));
        return result;
    }

public:
    Index() : current_doc_id(0) {}

    void add_document(const std::vector<std::string>& words) {
        for (const auto& word : words) {
            auto& postings = inverted_index[word];
            if (postings.empty() || postings.back() != current_doc_id) {
                postings.push_back(current_doc_id);
            }
        }
        current_doc_id++;
    }

    std::vector<int> get_postings(const std::string& word) const {
        auto it = inverted_index.find(word);
        if (it != inverted_index.end()) {
            return it->second;
        }
        return {};
    }

    int get_document_count() const {
        return current_doc_id;
    }

    std::vector<int> search(const QueryTree& query_tree) const {
        return evaluate_node(query_tree.getRoot());
    }

    std::vector<int> search(const std::string& query_string, bool ignore_case = true) const {
        QueryTree query_tree(query_string, ignore_case);
        return search(query_tree);
    }

    int count(const QueryTree& query_tree) const {
        return evaluate_node(query_tree.getRoot()).size();
    }

    int count(const std::string& query_string, bool ignore_case = true) const {
        return search(query_string, ignore_case).size();
    }

    void save(const std::string& filename) const {
        std::ofstream file(filename, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Could not open file for writing");
        }

        // Save current_doc_id
        file.write(reinterpret_cast<const char*>(&current_doc_id), sizeof(current_doc_id));

        // Save inverted_index
        size_t index_size = inverted_index.size();
        file.write(reinterpret_cast<const char*>(&index_size), sizeof(index_size));

        for (const auto& entry : inverted_index) {
            // Save word
            size_t word_length = entry.first.length();
            file.write(reinterpret_cast<const char*>(&word_length), sizeof(word_length));
            file.write(entry.first.c_str(), word_length);

            // Save postings list
            size_t postings_size = entry.second.size();
            file.write(reinterpret_cast<const char*>(&postings_size), sizeof(postings_size));
            file.write(reinterpret_cast<const char*>(entry.second.data()), postings_size * sizeof(int));
        }
    }

    void load(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Could not open file for reading");
        }

        // Clear existing data
        inverted_index.clear();

        // Load current_doc_id
        file.read(reinterpret_cast<char*>(&current_doc_id), sizeof(current_doc_id));

        // Load inverted_index
        size_t index_size;
        file.read(reinterpret_cast<char*>(&index_size), sizeof(index_size));

        for (size_t i = 0; i < index_size; ++i) {
            // Load word
            size_t word_length;
            file.read(reinterpret_cast<char*>(&word_length), sizeof(word_length));
            std::string word(word_length, '\0');
            file.read(&word[0], word_length);

            // Load postings list
            size_t postings_size;
            file.read(reinterpret_cast<char*>(&postings_size), sizeof(postings_size));
            std::vector<int> postings(postings_size);
            file.read(reinterpret_cast<char*>(postings.data()), postings_size * sizeof(int));

            inverted_index[word] = std::move(postings);
        }
    }

};