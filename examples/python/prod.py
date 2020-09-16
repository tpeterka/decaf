import pybredala as bd
import pydecaf as d
from mpi4py import MPI

w = d.Workflow()
w.makeWflow(w,"linear2.json")

a = MPI._addressof(MPI.COMM_WORLD)
r = MPI.COMM_WORLD.Get_rank()
decaf = d.Decaf(a,w)

for i in range(10):
	data = bd.SimpleFieldi(i)
	container = bd.pSimple()
	container.get().appendData("var", data, bd.DECAF_NOFLAG, bd.DECAF_PRIVATE, bd.DECAF_SPLIT_KEEP_VALUE, bd.DECAF_MERGE_ADD_VALUE)
	decaf.put(container,"out")

print("producer " + str(r) + " terminating")
decaf.terminate()
