import eldarcpp

original_query = ""
query_tree = eldarcpp.QueryTree(original_query)
print("Original query:", query_tree.to_string())

new_word = "kamala"

all_expansions = query_tree.generate_all_expansions(new_word)

for i, expanded_tree in enumerate(all_expansions, 1):
    print(f"\nExpansion {i}:")
    print("Query:", expanded_tree.to_string())

print(f"\nTotal number of expansions: {len(all_expansions)}")
