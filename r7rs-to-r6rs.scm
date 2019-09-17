(import (scheme base) (scheme file) (scheme read) (scheme write) (srfi 1))

(cond-expand (chibi (import (chibi show) (chibi show pretty)))
             (else))

(define substitutions
  '((arithmetic-shift bitwise-arithmetic-shift #f)
    (read-bytevector get-bytevector-n #t)
    (read-u8 get-u8 #f)
    (write-bytevector put-bytevector #t)
    (write-u8 put-u8 #t)))

(define (substitute form)
  (if (not (pair? form))
      form
      (let ((sub (assoc (car form) substitutions)))
        (if (not sub)
            (cons (substitute (car form))
                  (substitute (cdr form)))
            (let ((new-head (list-ref sub 1))
                  (reverse? (list-ref sub 2)))
              (cons new-head
                    (map substitute (if reverse?
                                        (reverse (cdr form))
                                        (cdr form)))))))))

(define (pretty-print out form)
  (cond-expand
    (chibi (show out (pretty form)))
    (else (write form out) (newline out))))

(define (read-all port)
  (let loop ((forms '()))
    (let ((form (read port)))
      (if (eof-object? form) forms (loop (append forms (list form)))))))

(define (r7rs->r6rs filename)
  (substitute
   (remove (lambda (form) (eqv? 'import (car form)))
           (call-with-input-file filename read-all))))

(define (write-r6rs-file filename . forms)
  (call-with-output-file filename
    (lambda (out)
      #;(display "#!r6rs\n" out)
      (display ";; Automatically generated\n" out)
      (for-each (lambda (form) (pretty-print out form)) forms))))

(define (translate-source-file r7rs-filename r6rs-filename)
  (apply write-r6rs-file r6rs-filename
         (cons '(import (rnrs) (ascii))
               (r7rs->r6rs r7rs-filename))))

(define (main)
  (let* ((lib (call-with-input-file "binary.sld" read))
         (libname (cadr lib))
         (exports (assoc 'export (cddr lib))))
    (write-r6rs-file "binary.sls"
                     `(library ,libname
                        ,exports
                        (import (rnrs))
                        ,@(r7rs->r6rs "binary.scm")))))

(main)
