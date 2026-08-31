# SPATTER: Natural Language Parsing as Statistical Pattern Recognition

An ISO C11 pure functional implementation of David M. Magerman's seminal statistical decision-tree parser:
> **David M. Magerman (1994 / 1995)**: *"Natural Language Parsing as Statistical Pattern Recognition"* (arXiv:cmp-lg/9405009 / ACL 1995 Proceedings).

---

## Key Features

1. **Pure Functional ISO C11 Architecture**:
   - Algebraic Data Types: `Option<T>` and `Result<T, Error>` for null-safe, exception-free computation.
   - Persistent Data Structures: Immutable cons-cell lists (`List`), immutable parse trees (`ParseNode`), and zero-copy string views (`StrView`).
   - Higher-Order Combinators: `map`, `filter`, `fold_left`, `fold_right`, `take`, and `and_then`.
   - Single-line Clang formatting compliance (`RULE[user_global]`).

2. **Statistical Decision Tree Models (SPATTER)**:
   - **POS-Tagging Model**: $P(T_i \mid \text{context})$ predicting part-of-speech categories.
   - **Node-Extension Model**: $P(E_i \mid \text{context})$ predicting constituent geometry (`EXT_RIGHT`, `EXT_LEFT`, `EXT_UP`, `EXT_UNARY`, `EXT_ROOT`).
   - **Node-Labeling Model**: $P(L_i \mid \text{context})$ predicting non-terminal phrase categories ($S, NP, VP, PP, \dots$).

3. **Tree Induction & Information Gain**:
   - Automated candidate binary question harvesting from corpus.
   - Shannon Entropy calculation: $H(S) = -\sum p_c \log_2 p_c$.
   - Information Gain optimization: $\Delta H(S, q) = H(S) - \sum \frac{|S_v|}{|S|} H(S_v)$.
   - Deleted interpolation / Laplace smoothing at decision tree leaves.

4. **Bottom-Up Beam Search Decoding**:
   - Hypotheses ranked by joint log-probability $\log P(T, W) = \sum \log P(a_i \mid H_i)$.
   - Dynamic pruning to top-$K$ active derivation paths.

5. **Penn Treebank I/O & PARSEVAL Evaluation**:
   - Native S-expression recursive descent parser.
   - Labeled Precision (LP), Labeled Recall (LR), F1-Score, and Exact Match tracking.

---

## Directory Layout

```
├── CMakeLists.txt              # CMake build configuration (C11)
├── Makefile                    # Standard Makefile
├── README.md                   # Project overview and instructions
├── docs/
│   ├── spatter_theory_and_architecture.md # Full theoretical documentation
│   └── functional_c11_specification.md    # C11 functional design specification
├── include/
│   └── spatter/
│       ├── common.h            # Allocation and utility macros
│       ├── option.h            # Option ADT and combinators
│       ├── result.h            # Result ADT and combinators
│       ├── str_view.h          # Immutable string slice operations
│       ├── list.h              # Persistent immutable list operations
│       ├── tree.h              # Parse tree nodes, tags, extensions, labels
│       ├── feature.h           # Context extraction and binary question predicates
│       ├── decision_tree.h     # Entropy, tree induction, smoothed inference
│       ├── model.h             # 3-model bundle (POS, Extension, Label)
│       ├── beam_search.h       # Beam search derivation state engine
│       ├── parser.h            # High-level sentence parsing pipeline
│       ├── treebank.h          # Penn Treebank S-expression parser & extraction
│       └── eval.h              # PARSEVAL metric calculations
├── src/
│   ├── option.c
│   ├── result.c
│   ├── str_view.c
│   ├── list.c
│   ├── tree.c
│   ├── feature.c
│   ├── decision_tree.c
│   ├── model.c
│   ├── beam_search.c
│   ├── parser.c
│   ├── treebank.c
│   ├── eval.c
│   └── main.c                 # Interactive CLI demonstration
├── tests/
│   ├── test_functional_primitives.c
│   ├── test_decision_tree.c
│   ├── test_spatter_parser.c
│   └── test_runner.c           # Automated test suite runner
└── data/
    ├── sample_train.penn       # Sample annotated training treebank
    └── sample_test.penn        # Sample annotated test treebank
```

---

## Building and Running

### Using CMake
```bash
cmake -B build -S .
cmake --build build --config Release
ctest --test-dir build --output-on-failure
./build/spatter_cli data/sample_train.penn data/sample_test.penn
```

### Using GCC / Clang Directly
```bash
# Compile and run automated test suite
gcc -std=c11 -Wall -Wextra -Wpedantic -O3 -Iinclude src/option.c src/result.c src/str_view.c src/list.c src/tree.c src/feature.c src/decision_tree.c src/model.c src/beam_search.c src/parser.c src/treebank.c src/eval.c tests/test_functional_primitives.c tests/test_decision_tree.c tests/test_spatter_parser.c tests/test_runner.c -o spatter_tests.exe -lm
./spatter_tests.exe

# Compile and run CLI demo
gcc -std=c11 -Wall -Wextra -Wpedantic -O3 -Iinclude src/*.c -o spatter_cli.exe -lm
./spatter_cli.exe data/sample_train.penn data/sample_test.penn
```

---

## References

1. **David M. Magerman (1994)**. *Natural Language Parsing as Statistical Pattern Recognition*. arXiv:cmp-lg/9405009.
2. **David M. Magerman (1995)**. *Statistical Decision-Tree Models for Parsing*. In Proceedings of the 33rd Annual Meeting of the Association for Computational Linguistics (ACL '95), pages 276–283.
