This is a tutorial on making a simple query language.

It is called "Monco" because it is a parody on Mongo written in C.

ROADMAP

[x] Nice README.txt.
[x] Storage of strings.
[x] REPL.
[x] Commands:
  - [x] add
  - [x] del
  - [x] search (substring)
  - [x] search with operators: &, |, ()
  - [x] search with not-operator: !
[ ] Storage of documents (similar to MongoDB) where values are strings.
[ ] More sophisticated types:
  - [ ] integers,
  - [ ] nested documents.
[ ] Query language similar to MongoDB:
  - [ ] select with $and, $or, $not,
  - [ ] add special type of filter with above operators.
