import pickle
f = open("file", "a+")
sample= 'asfasfd,ffaf '

value=['a',2,sample,4]
data=pickle.dumps(value)
print data

value=pickle.loads(data)
print value
f=value.pop(0)
print f