#!./scheme
486
(+ 137 349)
(- 1000 334)
(* 5 99)
(/ 10 2)
(+ 2.7 10)
(+ 21 35 12 7)
(* 25 4 12)
(+ (* 3 5) (- 10 6))
(+ (* 3 (+ (* 2 4) (+ 3 5))) (+ (- 10 7) 6))
(+ (* 3
	  (+ (* 2 4)
		 (+ 3 5)))
   (+ (- 10 7) 
	  6))
((lambda () 
   (define size 2)
   size
   (* 5 size)
))
((lambda ()
   (define pi 3.14159)
   (define radius 10)
   (* pi (* radius radius))
   (define circumference (* 2 pi radius))
   circumference
))
(define (square x) (* x x))
(square 21)
(square (+ 2 5))
(define (sum-of-squares x y)
  (+ (square x) (square y)))
(sum-of-squares 3 4)
(define (f a)
  (sum-of-squares (+ a 1) (* a 2)))
(f 5)
