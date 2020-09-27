select p.name
from Pokemon p,
     (select e.after_id candidate
      from Evolution e) r1
where r1.candidate not in (select e.before_id
      from Evolution e) and p.id=r1.candidate
order by p.name;