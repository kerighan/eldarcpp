import multiprocessing
from eldarcpp import Index, QueryTree


# Function to be executed in each process
def process_function(index_state, word):
    # Create a new Index instance from the state
    index = Index(index_state)
    # Perform operations on the Index
    query = QueryTree(word)
    res = index.count(query)
    print(f"Query for '{word}': {query.to_string()}, result: {res}")


if __name__ == "__main__":
    # Create an Index object and add a document
    index = Index()
    index.add_document(["word", "another", "word"])
    index.add_document(["word"])

    # Get the state of the index to share across processes
    index_state = index.get_state()

    # Create a pool of processes
    words = ["word", "another"]
    with multiprocessing.Pool(processes=2) as pool:
        # Run the process_function in parallel, passing the Index state
        pool.starmap(process_function, [(index_state, words[i]) for i in range(2)])
