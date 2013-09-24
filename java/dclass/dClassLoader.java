package dclass;

import java.io.InputStream;
import java.io.OutputStream;
import java.io.FileOutputStream;
import java.io.File;

public class dClassLoader
{
    public static boolean load(String lib)
    {
        if(sLoadLib(lib))
            return true;

        String os=detectOS();

        if(!os.isEmpty() && loadLibResource("/lib/",lib,os))
            return true;

        System.loadLibrary(lib);

        return false;
    }

    private static boolean sLoadLib(String lib)
    {
        try
        {
            System.loadLibrary(lib);
        }
        catch (UnsatisfiedLinkError ex)
        {
            return false;
        }

        return true;
    }

    private static boolean sLoad(String lib)
    {
        try
        {
            System.load(lib);
        }
        catch (UnsatisfiedLinkError ex)
        {
            return false;
        }

        return true;
    }

    private static boolean loadLibResource(String path,String lib,String os)
    {
        InputStream is=null;
        OutputStream fos=null;
        File f=null;

        //TODO fix this try catch
        try
        {
            is=dClassLoader.class.getResourceAsStream(path+lib+os);

            if(is==null)
                return false;

            f=File.createTempFile(lib,null);
            f.deleteOnExit();

            if(!f.exists())
                return false;

            fos=new FileOutputStream(f);

            byte[] buffer=new byte[10000];
            int read;

            while((read=is.read(buffer))!=-1)
            {
                fos.write(buffer,0,read);
            }

        }
        catch(Exception ex)
        {
            return false;
        }
        finally
        {
            try
            {
                is.close();
                fos.close();
            }
            catch(Exception ex) {}
        }

        return sLoad(f.getAbsolutePath());
    }

    private static String detectOS()
    {
        String os=System.getProperty("os.name").toLowerCase();
        String arch=System.getProperty("os.arch").toLowerCase();

        if(arch.equals("amd64"))
            arch="x86_64";
        else if(arch.equals("x86") || arch.equals("i686") || arch.equals("i386"))
            arch="x86_32";
        else if(arch.startsWith("armv7"))
            arch="armv7";

        if(os.contains("windows"))
            return "_win_"+arch+".dll";
        else if(os.contains("linux"))
            return "_linux_"+arch+".so";
        else if(os.contains("mac os"))
            return "_osx_"+arch+".dylib";

        return "";
    }
}
