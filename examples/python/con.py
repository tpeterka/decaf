import pybredala as bd
import pydecaf as d
from mpi4py import MPI

w = d.Workflow()
w.makeWflow(w,"linear2.json")

a = MPI._addressof(MPI.COMM_WORLD)
r = MPI.COMM_WORLD.Get_rank()
decaf = d.Decaf(a,w)

container = bd.pSimple()
while(decaf.get(container, "in")):
	recv = container.get().getFieldDataSI("var")
	res = recv.getData()
	print("consumer at rank " + str(r) + " received "+ str(res))

print("consumer at rank " + str(r) + " terminating")
