(defparameter *map*
  (list '(1 1 1 1 1 1 1 1 1 1 1)
        '(1 0 0 0 0 0 1 0 0 0 1)
        '(1 0 1 1 1 0 1 1 1 0 1)
        '(1 0 0 0 1 0 0 0 0 0 1)
        '(1 1 1 0 1 1 1 0 1 1 1)
        '(1 0 0 0 0 0 1 0 0 0 1)
        '(1 0 1 1 1 1 1 0 1 1 1)
        '(1 0 1 0 0 0 0 0 0 0 1)
        '(1 0 1 0 1 1 1 1 1 0 1)
        '(1 0 1 0 0 0 1 0 0 0 1)
        '(1 1 1 1 1 1 1 1 1 1 1)))

(defun used? (x y &key used)
  (not (not (find-if #'(lambda (e) (equal (list x y) e)) used))))

(defun check (x y &key (used nil))
  (and (= 0 (nth x (nth y *map*)))
       (not (used? x y :used used))))

(defun find-path (x1 y1 x2 y2 &key used)
  "(find-path 3 5 1 1)
   => ((3 5) (3 4) (3 3) (2 3) (1 3) (1 2) (1 1))"
  (let ((new-used (cons (list x1 y1) used)))
    (or
     (when (and (= x1 x2) (= y1 y2)) (list (list x1 y1)))
     (when (check (- x1 1) y1 :used new-used)
       (let ((r (find-path (- x1 1) y1 x2 y2 :used new-used)))
         (when r (cons (list x1 y1) r))))
     (when (check (+ x1 1) y1 :used new-used)
       (let ((r (find-path (+ x1 1) y1 x2 y2 :used new-used)))
         (when r (cons (list x1 y1) r))))
     (when (check x1 (- y1 1) :used new-used)
       (let ((r (find-path x1 (- y1 1) x2 y2 :used new-used)))
         (when r (cons (list x1 y1) r))))
     (when (check x1 (+ y1 1) :used new-used)
       (let ((r (find-path x1 (+ y1 1) x2 y2 :used new-used)))
         (when r (cons (list x1 y1) r)))))))