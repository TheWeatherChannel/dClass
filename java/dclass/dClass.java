package dclass;

import java.util.Map;
import java.util.HashMap;

public class dClass
{
    private String path;
    private long dclass_index;
    private int patterns;

    static
    {
        dClassLoader.load("dclassjava");
    }

    private native int init(String file);
    private native int write(long index,String file);
    private native long walk(long index);
    private native static String statversion();
    private native static String stataddressing();
    private native static long statnodesize();
    private native long statnodes(long index);
    private native long statmemory(long index);
    private native String comment(long index);
    private native void free(long index);
    private native long classify(long index,String s);
    private native long get(long index,String s);
    private native int kvlength(long kv);
    private native String kvgetid(long kv);
    private native String kvgetkey(long kv,int pos);
    private native String kvgetvalue(long kv,int pos);

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
    }

    public synchronized int init()
    {
        if(dclass_index==0)
        {
            patterns=init(path);

            return patterns;
        }
        else
            return -1;
    }

    public int getPatterns()
    {
        return patterns;
    }

    public long getNodes()
    {
        if(dclass_index!=0)
            return statnodes(dclass_index);
        else
            return -1;
    }

    public long getMemory()
    {
        if(dclass_index!=0)
            return statmemory(dclass_index);
        else
            return -1;
    }

    public String getComment()
    {
        if(dclass_index!=0)
            return comment(dclass_index);
        else
            return "";
    }

    public int write(String opath)
    {
        if(dclass_index!=0)
            return write(dclass_index,opath);
        else
            return -1;
    }

    public long walk()
    {
        if(dclass_index!=0)
            return walk(dclass_index);
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

        if(dclass_index!=0 && s!=null)
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

    public synchronized void free()
    {
        if(dclass_index!=0)
            free(dclass_index);
    }

    @Override
    protected void finalize() throws Throwable
    {
        free();
        super.finalize();
    }
}
