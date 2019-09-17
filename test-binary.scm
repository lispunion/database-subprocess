(import (scheme base) (scheme file) (scheme read) (scheme write) (binary))

(define tempfile "deleteme")

(define (writeln x)
  (write x)
  (newline))

(let ((old-code (call-with-input-file "binary.sls" read)))
  (call-with-port (open-binary-output-file tempfile)
                  (lambda (out) (write-binary-sexp out old-code)))
  (let ((new-code (call-with-port (open-binary-input-file tempfile)
                                  read-binary-sexp)))
    (writeln new-code)
    (writeln (equal? old-code new-code))))
