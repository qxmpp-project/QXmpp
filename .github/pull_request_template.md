Before opening a pull-request please do:
- [ ] Documentation:
  - [ ] Document every new public class and function
  - [ ] Add `\since QXmpp 1.X` to newly added classes and functions
  - [ ] Fix any doxygen warnings from your code (see log when building with `-DBUILD_DOCUMENTATION=ON`)
- [ ] When implementing or updating XEPs add it to `doc/xep.doc`
- [ ] Add unit tests for everything you've changed or added
- [ ] Run `clang-format -i <edited-file-1> ... <edited-file-n>` (replace `<edited-file-1> ... <edited-file-n>` by the files you edited)
