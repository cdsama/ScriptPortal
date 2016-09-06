print("hello world")

function add( a, b )
    return a + b
end

f.ID = 123;

print(f.ID)
print(f.ConstID)

function HandleError(err)
    print(err)
    print(debug.traceback())
end

function TestChangeConstID()
    f.ConstID = 456
end
print("-----------------")
xpcall(TestChangeConstID, HandleError)
print("-----------------")
vbar = test.Bar.GetBar()
print("-----------------")
print(vbar.name)
print("-----------------")