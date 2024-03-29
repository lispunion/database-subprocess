(load "binary.lisp")

(defun test ()
  (let ((stream (ext:make-pipe-io-stream "./driver-sqlite"
                                         :element-type '(unsigned-byte 8)
                                         :buffered t)))
    (labels ((command (form)
               (write-string "Q: ")
               (write form)
               (terpri)
               (write-binary-sexp stream form)
               (finish-output stream)
               (let ((response (read-binary-sexp stream)))
                 (write-string "A: ")
                 (write response)
                 ;;(show-stderr)
                 response)))
      (command `(|connect| |dbname| "test.db"))
      (command `(|disconnect|))
      (values))))

(test)
