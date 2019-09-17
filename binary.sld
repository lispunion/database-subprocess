(define-library (binary)
  (export read-binary-sexp
          write-binary-sexp)
  (import (scheme base)
          (scheme read)
          (scheme write))
  (cond-expand ((or chibi gauche sagittarius)
                (import (srfi 151))))
  (include "binary.scm"))
