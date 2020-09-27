select r.id, r.name, p2.name, p.name
from Evolution e, Pokemon p, Pokemon p2,
     (select e.after_id, p.id, p.name
     from Pokemon p, Evolution e
     where p.id=e.before_id) r
where r.after_id=e.before_id and p.id=e.after_id and p2.id=r.after_id;