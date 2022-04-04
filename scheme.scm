(define nil ())
(define (cadr l) (car (cdr l)))
(define (cddr l) (cdr (cdr l)))
(define (caddr l) (car (cddr l)))
(define (cdddr l) (cdr (cddr l)))
(define (list . l) l)
(define true #t)
(define false #f)
(define (null? a) (eq? a '()))

(define (@numeric-cascade op)
  (let ([fn #f])
    (set! fn (case-lambda
               [(a b) (op a b)]
               [(a b c . r) (and (op a b) (apply fn b c r))]))
    fn))

(define (@binary> a b) (@binary< b a))
(define (@binary<= a b) (not (@binary> a b)))
(define (@binary>= a b) (not (@binary< a b)))
(define < (@numeric-cascade @binary<))
(define > (@numeric-cascade @binary>))
(define <= (@numeric-cascade @binary<=))
(define >= (@numeric-cascade @binary>=))
(define = (@numeric-cascade @binary=))
(define eq? (@numeric-cascade @binary-eq?))
(define eqv? (@numeric-cascade @binary-eqv?))
(define (not a) (if a #f #t))
(define (abs x) (if (< x 0) (- x) x))
(define (even? x) (= (remainder x 2) 0))
(define (odd? x) (not (even? x)))

(define (@numeric-op op zero)
   (let ([fn #f])
     (set! fn (case-lambda
	        [() zero]
		[(a) (op zero a)]
		[(a b) (op a b)]
		[(a b c . r) (apply fn (op a b) c r)]
	      ))))

(define + (@numeric-op @binary+ 0))
(define - (@numeric-op @binary- 0))
(define * (@numeric-op @binary* 1))
(define / (@numeric-op @binary/ 1))

(define (map f ls . more)
  (if (null? more)
      (let map1 ([ls ls])
        (if (null? ls)
            '()
            (cons (f (car ls))
	          (map1 (cdr ls)))))
      (let map-more ([ls ls] [more more])
        (if (null? ls)
	    '()
	    (cons
	      (apply f (car ls) (map car more))
	      (map-more (cdr ls) (map cdr more)))))))

(define (exists f ls . more)
  (and (not (null? ls))
       (let exists_ ([x (car ls)] [ls (cdr ls)] [more more])
	 (if (null? ls)
	     (apply f x (map car more))
	     (or (apply f x (map car more))
	         (exists_ (car ls) (cdr ls) (map cdr more)))))))
