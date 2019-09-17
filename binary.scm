(define (read-varint-or-false in)
  (let loop ((value #f) (shift 0))
    (let ((byte (read-u8 in)))
      (if (eof-object? byte)
          value
          (let ((value (bitwise-ior (or value 0)
                                    (arithmetic-shift
                                     (bitwise-and #x7f byte) shift))))
            (if (= 0 (bitwise-and #x80 byte))
                value
                (loop value (+ shift 7))))))))

(define (read-varint in)
  (or (read-varint-or-false in)
      (error #f "eof when trying to read varint")))

(define (write-varint out value)
  (cond ((< value #x80)
         (write-u8 value out))
        (else
         (write-u8 (bitwise-ior #x80 (bitwise-and value #x7f)) out)
         (write-varint out (arithmetic-shift value -7)))))

(define (read-varbytes in)
  (let ((n (read-varint in)))
    (let ((b (read-bytevector n in)))
      (unless (= n (bytevector-length b))
        (error #f "Short read"))
      b)))

(define (write-varbytes out b)
  (write-varint out (bytevector-length b))
  (write-bytevector b out))

(define (read-binary-sexp in)
  (let ((tag (read-varint-or-false in)))
    (case tag
      ((#f)  (eof-object))
      ((#x0) '())
      ((#x1) #f)
      ((#x2) #t)
      ((#x3) (read-varbytes in))
      ((#x4) (read-varint in))
      ((#x5) (- (read-varint in)))
      ((#xc) (let* ((a (read-binary-sexp in))
                    (d (read-binary-sexp in)))
               (cons a d)))
      ((#xd) (let* ((n (read-varint in))
                    (v (make-vector n)))
               (let loop ((i 0))
                 (cond ((= i n) v)
                       (else (vector-set! v i (read-binary-sexp in))
                             (loop (+ i 1)))))))
      ((#xe) (utf8->string (read-varbytes in)))
      ((#xf) (string->symbol (utf8->string (read-varbytes in))))
      (else (error #f (string-append "Read unknown type tag: #x"
                                     (number->string tag)))))))

(define (write-binary-sexp out x)
  (cond ((null? x)
         (write-varint out 0))
        ((eqv? #f x)
         (write-varint out 1))
        ((eqv? #t x)
         (write-varint out 2))
        ((bytevector? x)
         (write-varint out 3)
         (write-varbytes out x))
        ((integer? x)
         (write-varint out (if (>= x 0) #x4 #x5))
         (write-varint out (abs x)))
        ((pair? x)
         (write-varint out #xc)
         (write-binary-sexp out (car x))
         (write-binary-sexp out (cdr x)))
        ((vector? x)
         (write-varint out #xd)
         (vector-for-each (lambda (elt) (write-binary-sexp out elt)) x))
        ((string? x)
         (write-varint out #xe)
         (write-varbytes out (string->utf8 x)))
        ((symbol? x)
         (write-varint out #xf)
         (write-varbytes out (string->utf8 (symbol->string x))))
        (else
         (error #f "Don't know how to write that kind of object"))))
