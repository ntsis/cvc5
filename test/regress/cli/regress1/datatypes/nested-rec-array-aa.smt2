; COMMAND-LINE: --arrays-exp
; EXPECT: unsat
(set-logic ALL)
(set-info :status unsat)
(set-option :dt-nested-rec true)
(declare-datatypes ((A 0)) (((v1) (v2) (v3 (value (Array A A)) ) ) ))
(define-fun term_array () (Array A A) ((as const (Array A A)) v1))
(define-fun term_a_array_updated () (Array A A) (store term_array v2 v2))
(define-fun assertion () Bool (= term_a_array_updated term_array))
(assert assertion)
(check-sat)
