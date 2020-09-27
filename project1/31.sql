select r.type
from (select p.type, count(*) as tcount
     from Pokemon p, Evolution e
     where p.id=e.before_id
     group by p.type) r
where r.tcount>=3
order by r.type desc;