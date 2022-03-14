#!/usr/bin/env scheme
((lambda ()
    (garbage-collect)
    (print 'post) (newline)))
