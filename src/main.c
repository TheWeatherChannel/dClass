/*
 * Copyright 2012 The Weather Channel
 * Copyright 2013 Reza Naghibi
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


extern void dtree_timersubn(struct timespec*,struct timespec*,struct timespec*);
extern int dtree_gettime(struct timespec*);


int main(int argc,char **args)
{
    int ret,i;
    int openddr=0;
    long count,ttime;
    char buf[1024];
    char *loadFile="../dtrees/openddr.dtree";
    char *outFile="";
    char *parameter=NULL;
    struct timespec startn,endn,diffn,total;
    dclass_index di;
    dtree_dt_index *h=&di.dti;
    const dclass_keyvalue *kvd;
    
    printf("dClass (version %s %s%zubit addressing %zubyte dt_node)\n",dclass_get_version(),
        DTREE_DT_PTR_TYPE,DTREE_DT_PTR_SIZE,sizeof(dtree_dt_node));
    
    for(i=1;i<argc;i++)
    {
        //-l [dtree file]
        if(!strcmp(args[i],"-l") && argc>(++i))
            loadFile=args[i];
        //-o [output dtree file]
        else if(!strcmp(args[i],"-o") && argc>(++i))
            outFile=args[i];
        //-m [open ddr resource dir]
        else if(!strcmp(args[i],"-d") && argc>(++i))
        {
            openddr=1;
            loadFile=args[i];
        }
        else if(!strncmp(args[i],"-h",2) || !strncmp(args[i],"--h",3))
        {
            printf("Usage: dclass_client [OPTIONS] [FILE|STRING]\n\n");
            printf("  -l <path>            load dtree from file\n");
            printf("  -o <path>            store dtree to file\n");
            printf("  -d <folder path>     load OpenDDR resources xmls\n");
            printf("  FILE                 STRING text file\n");
            printf("  STRING               test string\n");
            
            return 0;
        }
        //[test string] | [test file]
        else
            parameter=args[i];
    }

    printf("Loading %s: '%s'\n",openddr?"openddr":"dtree",loadFile);
    
    dtree_gettime(&startn);
    
    if(openddr)
        ret=openddr_load_resources(&di,loadFile);
    else
        ret=dclass_load_file(&di,loadFile);
    
    dtree_gettime(&endn);
    dtree_timersubn(&endn,&startn,&diffn);
    
    printf("load dtree tokens: %d time: %lds %ldms %ldus %ldns\n",
           ret,diffn.tv_sec,diffn.tv_nsec/1000000,diffn.tv_nsec/1000%1000,diffn.tv_nsec%1000);
    
    printf("dtree stats: nodes: %zu slabs: %zu mem: %zu bytes strings: %zu(%zu,%zu)\n",
           h->node_count,h->slab_count,h->size,h->dc_count,h->dc_slab_count,h->dc_slab_pos);
    
    printf("dtree comment: '%s'\n",(h->comment?h->comment:""));

    dtree_gettime(&startn);
    
    count=dtree_print(h,&dclass_get_id);
    
    dtree_gettime(&endn);
    dtree_timersubn(&endn,&startn,&diffn);
    
    printf("walk tree: %ld tokens %zu nodes time: %lds %ldms %ldus %ldns\n",
           count,h->node_count,diffn.tv_sec,diffn.tv_nsec/1000000,diffn.tv_nsec/1000%1000,diffn.tv_nsec%1000);
    
    if(*outFile)
    {
        printf("Dumping dtree: '%s'\n",outFile);
        
        ret=dclass_write_file(&di,outFile);
        
        printf("Wrote %d entries\n",ret);
    }
    
    dtree_gettime(&startn);
    
    kvd=dclass_classify(&di,"Mozilla/5.0 (Linux; U; Android 2.2; en; HTC Aria A6380 Build/ERE27) AppleWebKit/540.13+ (KHTML, like Gecko) Version/3.1 Mobile Safari/524.15.0");
    
    dtree_gettime(&endn);
    dtree_timersubn(&endn,&startn,&diffn);
    
    printf("HTC Aria UA lookup: '%s' time: %lds %ldms %ldus %ldns\n",
           kvd->id,diffn.tv_sec,diffn.tv_nsec/1000000,diffn.tv_nsec/1000%1000,diffn.tv_nsec%1000);
    
    if(parameter)
    {
        FILE *f;
        
        if((f=fopen(parameter,"r")))
        {
            printf("UA file: '%s'\n",parameter);
            
            count=0;
            
            memset(&total,0,sizeof(total));
            
            while(fgets(buf,sizeof(buf),f))
            {
#if DTREE_TEST_UALOOKUP
                int slen=strlen(buf)-1;
                
                if(buf[slen]=='\n')
                    buf[slen]='\0';
                
                printf("UA: '%s'\n",buf);
                
                fflush(stdout);
#endif

                dtree_gettime(&startn);
                
                kvd=dclass_classify(&di,buf);
                
                dtree_gettime(&endn);
                dtree_timersubn(&endn,&startn,&diffn);
                
                total.tv_sec+=diffn.tv_sec;
                total.tv_nsec+=diffn.tv_nsec;
                
                count++;
                
#if DTREE_TEST_UALOOKUP
                printf("UA lookup %ld: '%s' time: %lds %ldms %ldus %ldns\n",
                       count,kvd->id,diffn.tv_sec,diffn.tv_nsec/1000000,diffn.tv_nsec/1000%1000,diffn.tv_nsec%1000);
#endif
            }
            
            fclose(f);
            
            if(!count)
                count=1;
            
            ttime=(total.tv_sec*1000*1000*1000)+total.tv_nsec;
            ttime/=count;
            
            printf("TOTAL average time: %ld lookups, %lds %ldms %ldus %ldns\n",
                   count,ttime/1000000000,ttime/1000000%1000000,ttime/1000%1000,ttime%1000);
        }
        else
        {
            printf("UA: '%s'\n",parameter);
            
            dtree_gettime(&startn);
            
            kvd=dclass_classify(&di,parameter);
            
            dtree_gettime(&endn);
            dtree_timersubn(&endn,&startn,&diffn);
            
            printf("Param UA lookup: '%s' time: %lds %ldms %ldus %ldns\n",
                   kvd->id,diffn.tv_sec,diffn.tv_nsec/1000000,diffn.tv_nsec/1000%1000,diffn.tv_nsec%1000);
            
            if(kvd && kvd->size)
            {
                printf("OpenDDR attributes => ");
                for(i=0;i<kvd->size;i++)
                        printf("%s: '%s' ",kvd->keys[i],kvd->values[i]);
                printf("\n");
            }
        }
    }

    dclass_free(&di);
    
    return 0;
}
