(import (rnrs) (binary))

(define tempfile "deleteme")

(define (writeln x)
  (write x)
  (newline))

(let ((old-code (call-with-input-file "binary.sls" read)))
  (call-with-port (open-file-output-port tempfile (file-options no-fail))
                  (lambda (out) (write-binary-sexp out old-code)))
  (let ((new-code (call-with-port (open-file-input-port tempfile) read-binary-sexp)))
    (writeln new-code)
    (writeln (equal? old-code new-code))))
