(defconstant eof (gensym "EOF"))

(defconstant utf8-unix
  (ext:make-encoding
   :charset charset:utf-8
   :line-terminator :unix
   :input-error-action #\uFFFD
   :output-error-action :error))

(defun utf8->string (bytes)
  (ext:convert-string-from-bytes bytes utf8-unix))

(defun string->utf8 (string)
  (ext:convert-string-to-bytes string utf8-unix))

(defun read-varint-or-nil (in)
  (let ((byte (read-byte in nil nil)))
    (when byte
      (let ((value 0) (shift 0))
        (loop (setf value (logior value (ash (logand #x7f byte) shift)))
              (when (= 0 (logand #x80 byte)) (return value))
              (setf byte (read-byte in))
              (incf shift 7))))))

(defun read-varint (in)
  (or (read-varint-or-nil in) (error "eof when trying to read varint")))

(defun write-varint (out value)
  (cond ((< value #x80)
         (write-byte value out))
        (t
         (write-byte (logior #x80 (logand value #x7f)) out)
         (write-varint out (ash value -7)))))

(defun read-varbytes (in)
  (let* ((n (read-varint in))
         (b (make-array n)))
    (if (= n (read-sequence b in)) b (error "Short read"))))

(defun write-varbytes (out b)
  (write-varint out (length b))
  (write-sequence b out))

(defun read-binary-sexp (in)
  (let ((tag (read-varint-or-nil in)))
    (case tag
      ((nil) eof)
      (#x0 '())
      (#x1 'false)
      (#x2 'true)
      (#x3 (read-varbytes in))
      (#x4 (read-varint in))
      (#x5 (- (read-varint in)))
      (#xc (let* ((a (read-binary-sexp in))
                  (d (read-binary-sexp in)))
             (cons a d)))
      (#xd (let* ((n (read-varint in))
                  (v (make-array n)))
             (dotimes (i n v)
               (setf (aref v i) (read-binary-sexp in)))))
      (#xe (utf8->string (read-varbytes in)))
      (#xf (make-symbol (utf8->string (read-varbytes in))))
      (t   (error "Read unknown type tag: #x~2,'0X" tag)))))

(defun write-binary-sexp (out x)
  (cond ((null x)
         (write-varint out 0))
        ((eql 'false x)
         (write-varint out 1))
        ((eql 'true x)
         (write-varint out 2))
        ((integerp x)
         (write-varint out (if (>= x 0) #x4 #x5))
         (write-varint out (abs x)))
        ((consp x)
         (write-varint out #xc)
         (write-binary-sexp out (car x))
         (write-binary-sexp out (cdr x)))
        ((stringp x)
         (write-varint out #xe)
         (write-varbytes out (string->utf8 x)))
        ((symbolp x)
         (write-varint out #xf)
         (write-varbytes out (string->utf8 (symbol-name x))))
        ((vectorp x)
         (cond ((equal (array-element-type x) '(unsigned-byte 8))
                (write-varint out 3)
                (write-varbytes out x))
               (t
                (write-varint out #xd)
                (dotimes (i (length x))
                  (write-binary-sexp out (aref x i))))))
        (t
         (error "Don't know how to write that kind of object: ~S" x))))
