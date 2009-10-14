(register-al-plugin
 '((name "Al the Random Walker")
   (short-name "Random Walker")
   (author "Vytautas Jancauskas")
   (e-mail "unaudio@gmail.com")
   (min-tracks 1)
   (max-tracks 1)
   (handler random-walk-handler))
 '(("Trigger" #x0000 #x0001 hex-printer)
   ("Start" #x0000 #xFFFF hex-printer)
   ("Min" #x0000 #xFFFF hex-printer)
   ("Max" #x0000 #xFFFF hex-printer)
   ("Step" #x0000 #xFFFF hex-printer)))

(define (random-walk-handler parameters)
  (define (random-walk-stream start min max step)
    (cons-stream 
     start
     (let ((new-start (+ start (- (* (random step) 2) step -1))))
       (cond ((< new-start min) 
	      (random-walk-stream (+ min (- min new-start)) min max step))
	     ((> new-start max)
	      (random-walk-stream (- max (- new-start max)) min max step))
	     (else (random-walk-stream new-start min max step)))))))


























