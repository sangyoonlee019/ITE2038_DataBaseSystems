select p.name
from Pokemon p, Evolution e
where p.id=e.before_id and p.id>e.after_id
order by p.name;