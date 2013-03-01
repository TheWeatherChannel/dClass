package dclass;

import java.util.Map;
import java.util.HashMap;

public class dClass
{
    private String path;
    private long dclass_index;

    static
    {
        System.loadLibrary("dclassjava");
    }

    private native int init(String file);
    private native void free(long index);
    private native long classify(long index,String s);
    private native int kvlength(long kv);
    private native String kvgetid(long kv);
    private native String kvgetkey(long kv,int pos);
    private native String kvgetvalue(long kv,int pos);

    private dClass() { }

    public dClass(String s)
    {
        path=s;
        dclass_index=0;
    }

    public int init()
    {
        if(dclass_index==0)
            return init(path);
        else
            return -1;
    }

    public Map<String,String> classify(String s)
    {
        Map<String,String> ret=new HashMap<String,String>();
        if(dclass_index!=0)
        {
            long kv=classify(dclass_index,s);
            if(kv!=0)
            {
                int len=kvlength(kv);
                String id=kvgetid(kv);
                for(int i=0;i<len;i++)
                {
                    String key=kvgetkey(kv,i);
                    String value=kvgetvalue(kv,i);
                    ret.put(key,value);
                }
                ret.put("id",id);
            }
        }
        return ret;
    }

    public void free()
    {
        if(dclass_index!=0)
            free(dclass_index);
    }

    protected void finalize() throws Throwable
    {
        free();
        super.finalize();
    }

    public static void main(String[] args) throws Throwable
    {
        System.out.println("java dClass");

        if(args.length!=2)
        {
            System.out.println("usage: [path] [string]");
            return;
        }

        System.out.println("path: '"+args[0]+"'");

        dClass dc=new dClass(args[0]);

        int records=dc.init();

        System.out.println("init records: "+records);

        System.out.println("string: '"+args[1]+"'");

        Map<String,String> kv=dc.classify(args[1]);

        System.out.println("result size: "+kv.size());

        for(String key:kv.keySet())
        {
            String value=kv.get(key);
            System.out.println("key: '"+key+"' value: '"+value+"'");
        }

        dc.free();
    }
}
