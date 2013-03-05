package dclass;

import java.util.Map;
import java.util.HashMap;

public class dClass
{
    private String path;
    private long dclass_index;
    public int patterns;
    public long nodes;
    public long memory;

    static
    {
        dClassLoader.load("dclassjava");
    }

    private native int init(String file);
    private native static String statversion();
    private native static String stataddressing();
    private native static long statnodesize();
    private native long statnodes(long index);
    private native long statmemory(long index);
    private native void free(long index);
    private native long classify(long index,String s);
    private native long get(long index,String s);
    private native int kvlength(long kv);
    private native String kvgetid(long kv);
    private native String kvgetkey(long kv,int pos);
    private native String kvgetvalue(long kv,int pos);

    private dClass() { }

    public static String getVersion()
    {
        return statversion();
    }

    public static String getAddressing()
    {
        return stataddressing();
    }

    public static long getNodeSize()
    {
        return statnodesize();
    }

    public dClass(String s)
    {
        path=s;
        dclass_index=0;
        patterns=0;
        nodes=0;
        memory=0;
    }

    public int init()
    {
        if(dclass_index==0)
        {
            patterns=init(path);

            if(dclass_index!=0)
            {
                nodes=statnodes(dclass_index);
                memory=statmemory(dclass_index);
            }
            return patterns;
        }
        else
            return -1;
    }

    public Map<String,String> classify(String s)
    {
        return classify(s,true);
    }

    public Map<String,String> get(String s)
    {
        return classify(s,false);
    }

    private Map<String,String> classify(String s,boolean classify)
    {
        Map<String,String> ret=new HashMap<String,String>();

        if(dclass_index!=0)
        {
            long kv;

            if(classify)
                kv=classify(dclass_index,s);
            else
                kv=get(dclass_index,s);

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
}
