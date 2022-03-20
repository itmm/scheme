#!/usr/bin/env scheme
(define (compose f g)
  (lambda args
	(f (apply g args))))
((compose (lambda (x) (list 'foo x))
		  (lambda (x) (list 'bar x)))
 'x)

(define (iterate n)
  (lambda (f)
	(if (= n 0)
	  identity
	  (compose f ((iterate (- n 1)) f)))))
(define (identity x) x)
(define (square x) (* x x))
(((iterate 3) square) 5)
