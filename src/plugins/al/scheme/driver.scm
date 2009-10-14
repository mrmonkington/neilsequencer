(define (register-plugin properties))

;;; A function to print a specified number of stream elements
(define (print-stream stream elements)
  (if (not (= elements 0))
      (begin (display (head stream))
	     (newline)
	     (print-stream (tail stream) (- elements 1)))))
