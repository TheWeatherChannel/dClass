/*
 * Copyright 2012 The Weather Channel
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 */


#include "dclass_client.h"


//openddr property keys
static const char *openddr_key_array[] = OPENDDR_KEYS;


static int openddr_read_device_raw(FILE*,dtree_dt_index*,dtree_dt_index*);
static int openddr_read_pattern(FILE*,dtree_dt_index*,dtree_dt_index*,flag_f*);
static int openddr_add_device_pattern(dtree_dt_index*,dtree_dt_index*,dclass_keyvalue*,dclass_keyvalue*,char*,flag_f);
static const char *openddr_copy_string(dtree_dt_index*,const char*,const char*);
static int openddr_alloc_kvd(dtree_dt_index*,dtree_dt_index*,char*);
static char *openddr_get_attr(const char*,const char*,dtree_dt_index*,char**,char*);
static char *openddr_get_value(const char*,const char*,dtree_dt_index*,char**,char*);
static char *openddr_get_str(const char*,const char*,dtree_dt_index*,char**,char*,char*,char);
static char *openddr_unregex(const char*,char*);

#if OPENDDR_BLKBRY_FIX
static void openddr_convert_bb(char*);
#endif

extern char **dclass_get_value_pos(dclass_keyvalue*,char*);


//loads openddr resources
int openddr_load_resources(dclass_index *di,const char *path)
{
    int i;
    int ret;
    int dcount=0;
    int pcount=0;
    char fpath[1024];
    FILE *f=NULL;
    flag_f flags=0;
    dtree_dt_index dev;
    dtree_dt_index *h=&di->dti;

    dclass_init_index(di);
    dtree_init_index(&dev);
    
    h->sflags=DTREE_S_FLAG_REGEX|DTREE_S_FLAG_DUPS;
    
    //string and device cache
    dtree_alloc_string(h,"unknown",7);
    
    if(!openddr_alloc_kvd(h,&dev,"genericPhone") || !openddr_alloc_kvd(h,&dev,"genericTouchPhone") ||
            !openddr_alloc_kvd(h,&dev,"desktopDevice"))
        goto dexit;
    
    dtree_alloc_string(h,"true",4);
    dtree_alloc_string(h,"false",5);
    
    //build a device tree
    for(i=0;i<2;i++)
    {
        if(!i)
            sprintf(fpath,"%s/%s",path,OPENDDR_RSRC_DD);
        else
            sprintf(fpath,"%s/%s",path,OPENDDR_RSRC_DDP);

        dtree_printd(DTREE_PRINT_GENERIC,"OpenDDR device data: '%s'\n",fpath);

        if(!(f=fopen(fpath,"r")))
        {
            fprintf(stderr,"OpenDDR: cannot open '%s'\n",fpath);
            if(!i)
                goto dexit;
        }

        while((ret=openddr_read_device_raw(f,h,&dev)))
        {
            if(ret==-1)
                goto dexit;
            
            dcount++;
        }

        fclose(f);
    }
    
    dtree_printd(DTREE_PRINT_INITDTREE,"OpenDDR device data raw count: %d\n",dcount);
    
    dtree_printd(DTREE_PRINT_INITDTREE,"Open DDR raw dtree stats: nodes: %zu slabs: %zu mem: %zu bytes strings: %zu(%zu,%zu)\n",
           dev.node_count,dev.slab_count,dev.size,dev.dc_count,dev.dc_slab_count,dev.dc_slab_pos);
    
    //build main pattern tree
    for(i=0;i<2;i++)
    {
        if(!i)
            sprintf(fpath,"%s/%s",path,OPENDDR_RSRC_BD);
        else
            sprintf(fpath,"%s/%s",path,OPENDDR_RSRC_BDP);

        dtree_printd(DTREE_PRINT_GENERIC,"OpenDDR builder data: '%s'\n",fpath);

        if(!(f=fopen(fpath,"r")))
        {
            fprintf(stderr,"OpenDDR: cannot open '%s'\n",fpath);
            if(!i)
                goto dexit;
        }

        while((ret=openddr_read_pattern(f,h,&dev,&flags)))
        {
            if(ret==-1)
                goto dexit;
            
            pcount++;
        }

        fclose(f);
    }
    
    dtree_printd(DTREE_PRINT_INITDTREE,"OpenDDR pattern data raw count: %d\n",pcount);
    
    dtree_free(&dev);
    
    return pcount;
    
dexit:

    if(f)
        fclose(f);

    dtree_free(&dev);
    
    return -1;
}

//read a device from file, return it
static int openddr_read_device_raw(FILE *f,dtree_dt_index *h,dtree_dt_index *dev)
{
    int i;
    int ret=0;
    char buf[1048];
    char temp[DTREE_DATA_BUFLEN];
    dclass_keyvalue *kvd=NULL;
    
    if(!f)
        return 0;
    
    while((buf[sizeof(buf)-2]='\n') && fgets(buf,sizeof(buf),f))
    {
        //overflow
        if(buf[sizeof(buf)-2]!='\n' && buf[sizeof(buf)-2])
        {
            dtree_printd(DTREE_PRINT_INITDTREE,"OPENDDR LOAD: overflow detected\n");
            
            do
            {
                buf[sizeof(buf)-2]='\n';
                
                if(!fgets(buf,sizeof(buf),f))
                    break;
            }
            while(buf[sizeof(buf)-2]!='\n' && buf[sizeof(buf)-2]);
            
            continue;
        }
        
        if(strstr(buf,"<device "))
        {
            openddr_get_attr(buf,"id",NULL,NULL,temp);
            
            kvd=(dclass_keyvalue*)dtree_get(dev,temp,0);
            
            if(kvd)
                dtree_printd(DTREE_PRINT_INITDTREE,"OpenDDR device overwrite found: '%s'\n",temp);
            else
                kvd=(dclass_keyvalue*)dtree_alloc_mem(h,sizeof(dclass_keyvalue));
            
            if(!kvd)
                return -1;
            
            kvd->size=sizeof(openddr_key_array)/sizeof(*openddr_key_array);
            kvd->keys=openddr_key_array;
            
            kvd->values=(const char**)dtree_alloc_mem(h,sizeof(openddr_key_array));
            
            if(!kvd->values)
                return -1;
            
            kvd->id=dtree_alloc_string(h,temp,strlen(temp));
            
            openddr_get_attr(buf,"parentId",h,dclass_get_value_pos(kvd,"parentId"),NULL);
            
            ret=1;
            
            if(strstr(buf,"/>"))
                break;
        }
        else if(strstr(buf,"</device>"))
            break;
        else if(kvd && strstr(buf,"<property "))
        {
            openddr_get_attr(buf,"name",NULL,NULL,temp);
            
            openddr_get_attr(buf,"value",h,dclass_get_value_pos(kvd,temp),NULL);
        }
    }
    
    if(kvd)
    {
        dtree_printd(DTREE_PRINT_INITDTREE,"OpenDDR id(%p): '%s'(%p)\nAttributes => ",kvd,kvd->id,kvd->id);
        for(i=0;i<kvd->size;i++)
                dtree_printd(DTREE_PRINT_INITDTREE,"%s: '%s' ",kvd->keys[i],kvd->values[i]);
        dtree_printd(DTREE_PRINT_INITDTREE,"\n");

        //add to device tree
        if(dtree_add_entry(dev,kvd->id,kvd,0,NULL)<0)
            return -1;
    }
    
    return ret;
}

//read a pattern from file, insert it
static int openddr_read_pattern(FILE *f,dtree_dt_index *h,dtree_dt_index *dev,flag_f *flags)
{
    int ret=0;
    char buf[1048];
    char pattern[DTREE_DATA_BUFLEN];
    char id[DTREE_DATA_BUFLEN];
    dclass_keyvalue *der=NULL;
    dclass_keyvalue *chain=NULL;
    
#if OPENDDR_BLKBRY_FIX
    flag_f bb_flag;
#endif
    
    if(!f)
        return 0;
    
    while((buf[sizeof(buf)-2]='\n') && fgets(buf,sizeof(buf),f))
    {
        //overflow
        if(buf[sizeof(buf)-2]!='\n' && buf[sizeof(buf)-2])
        {
            dtree_printd(DTREE_PRINT_INITDTREE,"OPENDDR LOAD: overflow detected\n");
            
            do
            {
                buf[sizeof(buf)-2]='\n';
                
                if(!fgets(buf,sizeof(buf),f))
                    break;
            }
            while(buf[sizeof(buf)-2]!='\n' && buf[sizeof(buf)-2]);
            
            continue;
        }
        
        if(strstr(buf,"<device "))
        {
            openddr_get_attr(buf,"id",NULL,NULL,id);
            
            ret=1;
            
            //get device from device tree
            der=(dclass_keyvalue*)dtree_get(dev,id,0);
        }
        else if(strstr(buf,"<builder "))
        {
            if(strstr(buf,"TwoStepDeviceBuilder"))
                *flags=DTREE_DT_FLAG_CHAIN;
            else if(strstr(buf,"SimpleDeviceBuilder"))
                *flags=DTREE_DT_FLAG_WEAK;
        }
        else if(strstr(buf,"</builder>") && *flags)
            *flags=0;
        else if(strstr(buf,"</device>"))
            break;
        else if(der && strstr(buf,"<value>"))
        {
            openddr_get_value(buf,"value",NULL,NULL,pattern);
            
            dtree_printd(DTREE_PRINT_INITDTREE,"OpenDDR pattern: '%s' id: '%s' flags: %d (%p)\n",
                    pattern,der->id,*flags,der);
            
            if(!chain && *flags & DTREE_DT_FLAG_CHAIN)
            {
                //look for base chain
                chain=(dclass_keyvalue*)dtree_get(h,pattern,DTREE_DT_FLAG_BCHAIN);

                //TODO need to iterate thru regex and add all possible chain values
                if(!chain)
                    chain=(dclass_keyvalue*)dtree_get(h,openddr_unregex(pattern,id),DTREE_DT_FLAG_BCHAIN);

                //this pattern exists
                if(chain)
                {
                    dtree_printd(DTREE_PRINT_INITDTREE,"OpenDDR DUP CHAIN PATTERN: '%s' id: '%s'\n",pattern,der->id);

                    continue;
                }
            }
            
#if OPENDDR_BLKBRY_FIX
            bb_flag=*flags;
            
            if(!strncasecmp(der->id,"blackberry",10) && chain && (*flags & DTREE_DT_FLAG_CHAIN))
            {
                *flags=0;
                
                openddr_convert_bb(pattern);
            }
#endif
            
            if(openddr_add_device_pattern(h,dev,der,chain,pattern,*flags)<0)
                return -1;
            
#if OPENDDR_BLKBRY_FIX
            *flags=bb_flag;
#endif

            if(*flags & DTREE_DT_FLAG_CHAIN)
                chain=der;
        }
    }
    
    return ret;
}

//adds a device pattern to the root tree
static int openddr_add_device_pattern(dtree_dt_index *h,dtree_dt_index *dev,dclass_keyvalue *der,dclass_keyvalue *chain,char *pattern,flag_f flags)
{
    int i;
    
    //chain pattern
    if((flags & DTREE_DT_FLAG_CHAIN))
    {
        if(!chain)
            flags |= DTREE_DT_FLAG_BCHAIN;

        dtree_printd(DTREE_PRINT_INITDTREE,"OpenDDR CHAIN PATTERN: '%s' current: '%s' prev: '%s' flag: %u\n",
                pattern,der->id,chain?chain->id:"HEAD",flags);
    }
    
    for(i=0;i<der->size;i++)
    {
        if(!der->values[i])
            der->values[i]=openddr_copy_string(dev,dclass_get_kvalue(der,"parentId"),der->keys[i]);
    }
    
    return dtree_add_entry(h,pattern,der,flags,chain);
}

//alloc a device kvd
static int openddr_alloc_kvd(dtree_dt_index *h,dtree_dt_index *dev,char *id)
{
    dclass_keyvalue *kvd;
    
    kvd=(dclass_keyvalue*)dtree_alloc_mem(h,sizeof(dclass_keyvalue));
    
    if(!kvd)
        return 0;
    
    kvd->id=dtree_alloc_string(h,id,strlen(id));
    
    dtree_add_entry(dev,kvd->id,kvd,0,NULL);
    
    return 1;
}

//copy string from parent
static const char *openddr_copy_string(dtree_dt_index *h,const char *parent,const char *key)
{
    dclass_keyvalue *p;
    const char *ret;
    
    p=(dclass_keyvalue*)dtree_get(h,parent,0);
    
    if(!p)
        return NULL;

    ret=dclass_get_kvalue(p,key);
    
    if(!ret)
        return openddr_copy_string(h,dclass_get_kvalue(p,"parentId"),key);
    
    return ret;
}

//looks for attr in buf and copies it to dest
static char *openddr_get_attr(const char *buf,const char *attr,dtree_dt_index *h,char **vbuf,char *retbuf)
{
    return openddr_get_str(buf,attr,h,vbuf,retbuf,"=",'\"');
}

//looks for value in buf and copies it to dest
static char *openddr_get_value(const char *buf,const char *attr,dtree_dt_index *h,char **vbuf,char *retbuf)
{
    return openddr_get_str(buf,attr,h,vbuf,retbuf,">",'<');
}

//strcasestr
static const char *_alt_strcasestr(const char *haystack,const char *needle)
{
    const char *p;
    const char *startn=0;
    const char *np=0;

    for (p=haystack;*p;p++)
    {
        if(np)
        {
            if(toupper((unsigned char)*p)==toupper((unsigned char)*np))
            {
                if(!*++np)
                    return startn;
            }
            else
                np = 0;
        }
        else if(toupper((unsigned char)*p)==toupper((unsigned char)*needle))
        {
            np = needle + 1;
            startn = p;
        }
    }

    return 0;
}

//a generic str parser
static char *openddr_get_str(const char *buf,const char *attr,dtree_dt_index *h,char **vbuf,char *retbuf,char *suffix,char term)
{
    int i;
    const char *s;
    const char *e;
    char *ret=NULL;
    char temp[DTREE_DATA_BUFLEN];
    
    if(h && !vbuf)
        return NULL;
    
    i=sizeof(temp)-2-strlen(suffix);
    strncpy(temp,attr,i);
    temp[i+1]='\0';
    strcat(temp,suffix);
    
    s=_alt_strcasestr(buf,temp);
    
    if(s)
    {
        s+=strlen(temp);
        
        if(*s==term)
            s++;
        
        e=s;
        
        while(*e!=term)
        {
            //non quoted attribute
            if(*suffix=='=' && *e<=' ' && *(s-1)!='\"')
                break;
            e++;
        }
        
        i=e-s;
        
        if(i>=DTREE_DATA_BUFLEN)
            i=DTREE_DATA_BUFLEN-1;
        
        if(h)
        {
            ret=dtree_alloc_string(h,s,i);
            *vbuf=ret;
        }
        
        if(retbuf)
        {   
            ret=strncpy(retbuf,s,i);
            retbuf[i]='\0';
        }
    }
    
    return ret;
}

//removes regex
static char *openddr_unregex(const char *s,char *dest)
{
    int h=0;
    char *d=dest;
    
    while(*s)
    {
        if(*s==DTREE_PATTERN_SET_S)
        {
            *d++=*++s;
            h=1;
            continue;
        }
        else if(*s==DTREE_PATTERN_SET_E)
        {
            s++;
            h=0;
            continue;
        }
        else if(*s==DTREE_PATTERN_OPTIONAL)
        {
            s++;
            continue;
        }
        if(!h)
            *d++=*s++;
        else
            s++;
    }
    
    *d='\0';
    
    return dest;
}

#if OPENDDR_BLKBRY_FIX
//blackberry pattern fix
static void openddr_convert_bb(char *pattern)
{
    char *orig=pattern;
    char new[DTREE_DATA_BUFLEN];
    
    *new='\0';
    
    if(strncasecmp(orig,"blackberry",10))
        return;
    
    strcat(new,"blackberry.?");
    
    orig+=10;
    
    if(*orig==' ')
        orig++;
    
    if(strlen(orig)>8)
        return;
    
    strcat(new,orig);
    
    strcat(new,".?");
    
    strncpy(pattern,new,DTREE_DATA_BUFLEN-1);
    
    pattern[DTREE_DATA_BUFLEN-1]='\0';
}
#endif
