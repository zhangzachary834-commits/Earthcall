# TestLabAI

This folder contains a toy language model written in plain C++ for learning purposes.

It is not a real large language model. It is a small word-level generator that teaches the core loop:

1. Read training text.
2. Break the text into tokens.
3. Count which words follow which other words.
4. Use those counts as probabilities.
5. Sample the next word repeatedly to generate output.

## Files

- `main.cpp`: loads training text, trains the model, and runs a prompt loop.
- `ToyLanguageModel.hpp`: the model interface.
- `ToyLanguageModel.cpp`: tokenization, training, and generation logic.
- `corpus.txt`: the training data.

## How the model works

### 1. Tokenization

The code splits text into words using spaces, then normalizes each token by:

- lowercasing it
- removing punctuation except apostrophes

This gives the model a simpler vocabulary to learn from.

### 2. Training

During training, the model builds a table like this:

- `"language" -> {"models": 2, "can": 1}`
- `"models" -> {"predict": 1, "scale": 1}`

That table is called a transition map. It stores how often one word is followed by another.

### 3. Generation

When you enter a prompt, the model:

1. Takes the last word of your prompt.
2. Looks up which words usually come next.
3. Randomly picks one, weighted by frequency.
4. Repeats the process until it hits an end marker or the token limit.

This is the same broad shape as language modeling: predict the next token from context.

## Why this is not an LLM

Real LLMs are different in major ways:

- They use subword tokenization, not simple whitespace splitting.
- They learn weights with gradient descent, not raw counting.
- They use neural networks and attention, not a transition table.
- They handle much longer context.
- They train on massive datasets.

This project is still useful because it teaches the core mental model before the math and scale become complex.

## Build and run

```bash
cd /Users/zachary/Documents/GitHub/Earthcall/TestLabAI
clang++ -std=c++17 -Wall -Wextra -pedantic main.cpp ToyLanguageModel.cpp -o testlab_ai
./testlab_ai
```

## Good next upgrades

- Replace word tokens with character tokens.
- Add trigram context instead of only one previous word.
- Save the trained transition table to disk.
- Score multiple candidate next words and print their probabilities.
- Rebuild the model in Python, then compare the code to a small neural model.
