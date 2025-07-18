Java.perform(function(){
    function show_stacks(){
        console.log(
            Java.use("android.util.Log").getStackTraceString(
                Java.use("java.lang.Throwable").$new()
            )
        );
    }

    var JSONObject = Java.use('org.json.JSONObject');
    JSONObject.put.overload('java.lang.String', 'java.lang.Object').implementation = function (a, b) {
        console.log('jsonobj.put Object ' + a + b);
        return this.put(a, b);
    }
});