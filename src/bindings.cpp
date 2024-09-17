#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "index.h"
#include "query_tree.h"
#include <iostream>

namespace py = pybind11;

PYBIND11_MODULE(eldarcpp, m) {
    py::class_<Index>(m, "Index")
        .def(py::init<>())
        .def("add_document", &Index::add_document)
        .def("get_postings", &Index::get_postings)
        .def("get_document_count", &Index::get_document_count)
        .def("search", py::overload_cast<const QueryTree&>(&Index::search, py::const_))
        .def("search", py::overload_cast<const std::string&, bool>(&Index::search, py::const_),
             py::arg("query_string"), py::arg("ignore_case") = true)
        .def("count", py::overload_cast<const QueryTree&>(&Index::count, py::const_))
        .def("count", py::overload_cast<const std::string&, bool>(&Index::count, py::const_),
             py::arg("query_string"), py::arg("ignore_case") = true)
        .def("save", &Index::save)
        .def("load", &Index::load);

    py::class_<QueryTree>(m, "QueryTree")
        .def(py::init<const std::string&, bool>(), 
             py::arg("query"), py::arg("ignore_case") = true)
        .def("to_string", &QueryTree::toString, py::arg("flattened") = true)
        .def("get_root", &QueryTree::getRoot, py::return_value_policy::reference_internal)
        .def("expand", &QueryTree::expand, py::arg("path"), py::arg("new_word"), py::arg("op") = "AND")
        .def("generate_all_expansions", &QueryTree::generateAllExpansions)
        .def("__repr__", &QueryTree::repr);

    // Add bindings for QueryNode and its derived classes
    py::class_<QueryNode, std::unique_ptr<QueryNode>>(m, "QueryNode")
        .def("to_string", &QueryNode::toString);

    py::class_<WordNode, QueryNode>(m, "WordNode")
        .def("get_word", &WordNode::getWord);

    py::class_<NotNode, QueryNode>(m, "NotNode")
        .def("get_child", &NotNode::getChild, py::return_value_policy::reference_internal);

    py::class_<BinaryOpNode, QueryNode>(m, "BinaryOpNode")
        .def("get_left", &BinaryOpNode::getLeft, py::return_value_policy::reference_internal)
        .def("get_right", &BinaryOpNode::getRight, py::return_value_policy::reference_internal);

    py::class_<AndNode, BinaryOpNode>(m, "AndNode");
    py::class_<OrNode, BinaryOpNode>(m, "OrNode");
    py::class_<AndNotNode, BinaryOpNode>(m, "AndNotNode");
}