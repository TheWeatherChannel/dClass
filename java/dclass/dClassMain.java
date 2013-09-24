package dclass;

import java.util.Map;
import java.io.File;
import java.io.BufferedReader;
import java.io.FileReader;

public class dClassMain
{
    private static final boolean DTREE_LOG_UALOOKUP=true;

    public static void main(String[] args) throws Throwable
    {
        int ret,i;
        int argc=args.length;
        int openddr=0;
        long count,total;
        long startn,diffn;
        String loadFile="../dtrees/openddr.dtree";
        String outFile=null;
        String parameter=null;
        dClass di;
        Map<String,String> kv;

        System.out.println("dClass (version "+dClass.getVersion()+" "+dClass.getAddressing()+"bit addressing "+dClass.getNodeSize()+"byte dt_node)");

        for(i=0;i<argc;i++)
        {
            //-l [dtree file]
            if(args[i].equals("-l") && argc>(++i))
                loadFile=args[i];
            //-o [output dtree file]
            else if(args[i].equals("-o") && argc>(++i))
                outFile=args[i];
            //-m [open ddr resource dir]
            else if(args[i].equals("-d") && argc>(++i))
            {
                openddr=1;
                loadFile=args[i];
            }
            else if(args[i].startsWith("-h") || args[i].startsWith("--h"))
            {
                System.out.println("Usage: dclass_client [OPTIONS] [FILE|STRING]\n");
                System.out.println("  -l <path>            load dtree from file");
                System.out.println("  -o <path>            store dtree to file");
                System.out.println("  -d <folder path>     load OpenDDR resources xmls");
                System.out.println("  FILE                 STRING text file");
                System.out.println("  STRING               test string");
                
                return;
            }
            //[test string] | [test file]
            else
                parameter=args[i];
        }

        System.out.println("Loading "+(openddr>0?"openddr":"dtree")+": '"+loadFile+"'");

        startn=System.nanoTime();

        di=new dClass(loadFile);

        ret=di.init();

        diffn=System.nanoTime()-startn;

        System.out.println("load dtree tokens: "+ret+" time: "+getTime(diffn));

        System.out.println("dtree stats: nodes: "+di.getNodes()+" mem: "+di.getMemory());

        System.out.println("dtree comment: '"+di.getComment()+"'");

        startn=System.nanoTime();

        ret=(int)di.walk();

        diffn=System.nanoTime()-startn;

        System.out.println("walk tree: "+ret+" tokens "+di.getNodes()+" nodes time: "+getTime(diffn));

        if(outFile!=null)
        {
            System.out.println("Dumping dtree: '"+outFile+"'");
        
            ret=di.write(outFile);
        
            System.out.println("Wrote "+ret+" entries");
        }

        String test="Mozilla/5.0 (Linux; U; Android 2.2; en; HTC Aria A6380 Build/ERE27) AppleWebKit/540.13+ (KHTML, like Gecko) Version/3.1 Mobile Safari/524.15.0";

        startn=System.nanoTime();

        kv=di.classify(test);

        diffn=System.nanoTime()-startn;

        System.out.println("HTC Aria lookup: '"+kv.get("id")+"' time: "+getTime(diffn));

        if(parameter==null);
        else if((new File(parameter)).exists())
        {
            System.out.println("UA file: '"+parameter+"'");

            count=0;
            total=0;

            try
            {
                BufferedReader in=new BufferedReader(new FileReader(parameter));
                String line;

                while((line=in.readLine())!=null)
                {
                    if(DTREE_LOG_UALOOKUP)
                        System.out.println("UA: '"+line+"'");

                    startn=System.nanoTime();

                    kv=di.classify(line);

                    diffn=System.nanoTime()-startn;

                    total+=diffn;
                    count++;

                    if(DTREE_LOG_UALOOKUP)
                        System.out.println("UA lookup "+count+": '"+kv.get("id")+"' time: "+getTime(diffn));
                }

                in.close();
            }
            catch(Exception ex)
            {
            }

            if(count==0)
                count=1;

            total/=count; 

            System.out.println("TOTAL average time: "+count+", "+getTime(diffn));
        }
        else
        {
            System.out.println("UA: '"+parameter+"'");

            startn=System.nanoTime();

            kv=di.classify(parameter);

            diffn=System.nanoTime()-startn;

            System.out.println("Param UA lookup: '"+kv.get("id")+"' time: "+getTime(diffn));

            System.out.print("OpenDDR attributes => ");

            for(String key:kv.keySet())
            {
                String value=kv.get(key);
                System.out.print(key+": '"+value+"' ");
            }

            System.out.println("");
        }

        di.free();
    }

    public static String getTime(long diffn)
    {
        return diffn/(1000*1000*1000)+"s "+diffn/(1000*1000)%1000+"ms "+diffn/1000%1000+"us "+diffn%1000+"ns";
    }
}
