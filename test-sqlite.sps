(import (chezscheme) (binary))

(define dbname "test.db")

(define (displayln x)
  (display x)
  (newline))

(define (writeln x)
  (write x)
  (newline))

(let-values (((to-sub from-sub sub-stderr process-id)
              (open-process-ports "exec ./driver-sqlite")))

  (define (show-stderr)
    (when (input-port-ready? sub-stderr)
      (displayln (utf8->string (get-bytevector-some sub-stderr)))))

  (define (command form)
    (display "Q: ")
    (writeln form)
    (write-binary-sexp to-sub form)
    (flush-output-port to-sub)
    (let ((response (read-binary-sexp from-sub)))
      (display "A: ")
      (writeln response)
      response))

  (command `(connect dbname ,dbname))
  (command `(execute "create table hello (greeting text)"))
  (command `(execute "insert into hello (greeting) values ('Hello world')"))
  (command `(disconnect))

  (show-stderr))
