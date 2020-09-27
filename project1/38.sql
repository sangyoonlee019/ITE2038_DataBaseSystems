select distinct p.name
from Evolution e,Pokemon p,
     (select e.after_id
      from Evolution e) r
where r.after_id<>e.before_id and p.id=r.after_id
order by p.name;