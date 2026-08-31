# Functional C11 Architecture Specification

This specification defines the functional programming guidelines, memory management model, and Clang single-line code formatting rules for the SPATTER parser implementation.

---

## 1. Core Functional Conventions in C11

### 1.1 Pure Functions & Immutability
- Functions must not modify arguments in-place unless an explicit builder is requested.
- Data structures are returned as `const` pointers to heap allocations or by value.
- State is transitioned by creating new state instances:
  $$\text{next\_state} = f(\text{current\_state}, \text{action})$$

### 1.2 Algebraic Data Types
- **`Option`**: Encapsulates nullable results.
  ```c
  typedef struct Option {
      bool is_some;
      void* value;
  } Option;
  ```
- **`Result`**: Encapsulates fallible operations without exceptions or errno.
  ```c
  typedef struct Result {
      bool is_ok;
      union { void* ok; const char* err; } as;
  } Result;
  ```

### 1.3 Persistent Data Structures
- **Immutable Singly-Linked List (`List`)**:
  ```c
  typedef struct ListNode {
      const void* head;
      const struct ListNode* tail;
  } ListNode;
  ```
  Functions like `list_cons`, `list_map`, `list_filter`, and `list_fold_left` return new list heads without mutating existing nodes.

---

## 2. Clang Formatting Rules (`RULE[user_global]`)

All code strictly conforms to the following rules:

### 2.1 Function Declarations & Signatures
- Every function declaration and definition MUST start and end its parameter list on the **same single line**:
  ```c
  /* Correct */
  double calculate_entropy(const size_t* counts, size_t num_classes, size_t total_samples)
  {
      /* Body */
  }

  /* FORBIDDEN */
  double calculate_entropy(
      const size_t* counts,
      size_t num_classes,
      size_t total_samples
  ) {
      /* Body */
  }
  ```

### 2.2 Variable Declarations
- Variable declarations and initializations MUST NOT be split across multiple lines:
  ```c
  /* Correct */
  float e = 1.3f;
  int m = 2;
  const ParseNode* node = parse_node_create(word, tag, ext, lbl);

  /* FORBIDDEN */
  float e =
      1.3f;
  int m =
      2;
  ```

### 2.3 Loops and Arrays
- Loop headers and array initializers must avoid unnecessary line breaks.

---

## 3. Module Hierarchy

1. **`spatter/common.h`**: Common macros, types, memory allocation wrappers.
2. **`spatter/option.h`**: Monadic `Option` type and combinators.
3. **`spatter/result.h`**: Monadic `Result` type and combinators.
4. **`spatter/str_view.h`**: Zero-allocation immutable string slices.
5. **`spatter/list.h`**: Pure persistent cons-list and higher-order combinators (`map`, `filter`, `fold`, `take`).
6. **`spatter/tree.h`**: Parse tree nodes, tags, extensions, labels, and Penn bracketed tree rendering.
7. **`spatter/feature.h`**: Context extraction and binary question predicate evaluators.
8. **`spatter/decision_tree.h`**: Shannon entropy calculation, information gain, binary tree training, and smoothed leaf prediction.
9. **`spatter/model.h`**: The 3 unified SPATTER models (POS tagger, Extension classifier, Label classifier).
10. **`spatter/beam_search.h`**: Beam search state tracking and top-$K$ hypothesis pruning.
11. **`spatter/parser.h`**: End-to-end functional parsing pipeline.
12. **`spatter/treebank.h`**: S-expression Treebank parser and training example extractor.
13. **`spatter/eval.h`**: PARSEVAL metric calculations (Precision, Recall, F1).
