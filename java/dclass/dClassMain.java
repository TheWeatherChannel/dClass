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
        long startn,endn,diffn;
        String loadFile="../dtrees/openddr.dtree";
        String outFile="";
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

        endn=System.nanoTime();
        diffn=endn-startn;

        System.out.println("load dtree tokens: "+ret+" time: "+diffn/(1000*1000*1000)+"s "+
            diffn/(1000*1000)%(1000*1000)+"ms "+diffn/1000%1000+"us "+diffn%1000+"ns");

        System.out.println("dtree stats: nodes: "+di.nodes+" mem: "+di.memory);

        String test="Mozilla/5.0 (Linux; U; Android 2.2; en; HTC Aria A6380 Build/ERE27) AppleWebKit/540.13+ (KHTML, like Gecko) Version/3.1 Mobile Safari/524.15.0";

        startn=System.nanoTime();

        kv=di.classify(test);

        endn=System.nanoTime();
        diffn=endn-startn;

        System.out.println("HTC Aria lookup: '"+kv.get("id")+"' time: "+diffn/(1000*1000*1000)+"s "+
            diffn/(1000*1000)%(1000*1000)+"ms "+diffn/1000%1000+"us "+diffn%1000+"ns");

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

                    endn=System.nanoTime();
                    diffn=endn-startn;

                    total+=diffn;
                    count++;

                    if(DTREE_LOG_UALOOKUP)
                        System.out.println("UA lookup "+count+": '"+kv.get("id")+"' time: "+diffn/(1000*1000*1000)+"s "+
                            diffn/(1000*1000)%(1000*1000)+"ms "+diffn/1000%1000+"us "+diffn%1000+"ns");
                }

                in.close();
            }
            catch(Exception ex)
            {
            }

            if(count==0)
                count=1;

            total/=count; 

            System.out.println("TOTAL average time: "+count+", "+total/(1000*1000*1000)+"s "+
                total/(1000*1000)%(1000*1000)+"ms "+total/1000%1000+"us "+total%1000+"ns");
        }
        else
        {
            System.out.println("UA: '"+parameter+"'");

            startn=System.nanoTime();

            kv=di.classify(parameter);

            endn=System.nanoTime();
            diffn=endn-startn;

            System.out.println("Param UA lookup: '"+kv.get("id")+"' time: "+diffn/(1000*1000*1000)+"s "+
                diffn/(1000*1000)%(1000*1000)+"ms "+diffn/1000%1000+"us "+diffn%1000+"ns");

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
}
