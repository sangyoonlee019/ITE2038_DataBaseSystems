select p.type, count(*) as tcount
from Pokemon p
group by p.type
order by tcount,p.type;