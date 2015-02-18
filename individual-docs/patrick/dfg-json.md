
```
#!json

{
    "nodes" : [
	{ "name" : "A", "master" : true, "source" : true },
	{ "name" : "B", "operation" : "rot13" },
	{ "name" : "C", "sink" : true }
    ],
    "links" : [
	{ "source": 1, "target": 2 },
	{ "source": 2, "target": 3 }
    ]
}

```
