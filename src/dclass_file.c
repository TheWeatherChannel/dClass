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


//structs for parsing dtree file
typedef struct
{
    char *key;
    char *value;
    
    size_t key_len;
    size_t val_len;
} dtree_kv_pair;

typedef struct
{
    char *pattern;
    char *id;
    char *cparam;
    char *type;
    
    size_t id_len;
    size_t cparam_len;
    
    int count;
    int legacy;
    
    flag_f flag;
    
    dtree_kv_pair p[DTREE_DATA_MKEYS];
} dtree_file_entry;


static void dclass_parse_fentry(char*,dtree_file_entry*);
static long dclass_write_tree(const dtree_dt_index*,const dtree_dt_node*,char*,int,FILE*);
static void dclass_write_node(const dtree_dt_node*,char*,FILE*);


#if DTREE_PERF_WALKING
extern void dtree_timersubn(struct timespec*,struct timespec*,struct timespec*);
#endif


//parses a dtree file
int dclass_load_file(dclass_index *di,const char *path)
{
    int i,j;
    int ret;
    int lines=0;
    int keys_size=0;
    char *p;
    char *s;
    char **keys=NULL;
    char buf[10*1024];
    FILE *f;
    dtree_dt_index ids;
    dtree_dt_index *h=&di->dti;
    dtree_file_entry fe;
    dclass_keyvalue *kvd;
    dclass_keyvalue *cparam;
    
#if DTREE_PERF_WALKING
    struct timespec startn,endn,diffn;
    long total,high,low,val,lret;
#endif
    
    dclass_init_index(di);
    dtree_init_index(&ids);
    
    dtree_printd(DTREE_PRINT_GENERIC,"DTREE: Loading '%s'\n",path);
    
    if(!(f=fopen(path,"r")))
    {
        fprintf(stderr,"DTREE: cannot open '%s'\n",path);
        return -1;
    }
    
    //traverse the loadfile
    while((buf[sizeof(buf)-2]='\n') && (p=fgets(buf,sizeof(buf),f)))
    {
        lines++;
        
        //overflow
        if(buf[sizeof(buf)-2]!='\n' && buf[sizeof(buf)-2])
        {
            dtree_printd(DTREE_PRINT_INITDTREE,"LOAD: overflow detected on line %d\n",lines);
            
            do
            {
                buf[sizeof(buf)-2]='\n';
                
                if(!fgets(buf,sizeof(buf),f))
                    break;
            }
            while(buf[sizeof(buf)-2]!='\n' && buf[sizeof(buf)-2]);
            
            continue;
        }
        
        if(*buf=='#')
        {
            switch(*++p)
            {
                //string order is being forced here
                case '!':
                    for(++p,s=p;*p;p++)
                    {
                        if(*p==',' || !*(p+1))
                        {
                            *p='\0';
                            s=dtree_alloc_string(h,s,p-s);
                            dtree_printd(DTREE_PRINT_INITDTREE,"INIT FORCE string: '%s'\n",s);
                            
                            kvd=(dclass_keyvalue*)dtree_alloc_mem(h,sizeof(dclass_keyvalue));
            
                            if(!kvd)
                                goto lerror;

                            kvd->id=s;

                            if(dtree_add_entry(&ids,kvd->id,kvd,0,NULL)<0)
                                goto lerror;

                            dtree_printd(DTREE_PRINT_INITDTREE,"IDS: Successful add: '%s'\n",kvd->id);
                            
                            s=(p+1);
                        }
                    }
                    break;
                //index params
                case '$':
                    p++;
                    if(!strncasecmp(p,"partial",6))
                    {
                        h->sflags |= DTREE_S_FLAG_PARTIAL;
                        dtree_printd(DTREE_PRINT_INITDTREE,"INIT FORCE: partial\n");
                    }
                    else if(!strncasecmp(p,"nopart",6))
                    {
                        h->sflags |= DTREE_S_FLAG_NOPART;
                        dtree_printd(DTREE_PRINT_INITDTREE,"INIT FORCE: no partial\n");
                    }
                    else if(!strncasecmp(p,"regex",5))
                    {
                        h->sflags |= DTREE_S_FLAG_REGEX;
                        dtree_printd(DTREE_PRINT_INITDTREE,"INIT FORCE: regex\n");
                    }
                    else if(!strncasecmp(p,"dups",4))
                    {
                        h->sflags |= DTREE_S_FLAG_DUPS;
                        dtree_printd(DTREE_PRINT_INITDTREE,"INIT FORCE: dups\n");
                    }
                    break;
            }          
            continue;
        }
        
        memset(&fe,0,sizeof(fe));
        
        //parse the line
        dclass_parse_fentry(buf,&fe);
        
        if((!fe.count && fe.legacy) || (!fe.id && !fe.legacy))
        {
            dtree_printd(DTREE_PRINT_INITDTREE,"LOAD: bad line detected: '%s':%d\n",buf,lines);
            continue;
        }
                
        //normalize legacy fe
        if(fe.legacy)
        {
            for(i=0;i<fe.count;i++)
            {
                s=dtree_alloc_string(h,fe.p[i].key,fe.p[i].key_len);
                if(!i || DTREE_DC_DISTANCE(h,s)<DTREE_DC_DISTANCE(h,(char*)fe.id))
                {
                    fe.id=s;
                    fe.id_len=fe.p[i].key_len;
                    fe.type=fe.p[i].value;
                }

            }
            
            fe.count=0;
            
            if(!(h->sflags & DTREE_S_FLAG_NOPART))
                h->sflags |= DTREE_S_FLAG_PARTIAL;
        }
        
        if(fe.type)
        {
            if(*fe.type=='S')
                fe.flag=DTREE_DT_FLAG_STRONG;
            else if(*fe.type=='W')
                fe.flag=DTREE_DT_FLAG_WEAK;
            else if(*fe.type=='P')
                fe.flag=DTREE_DT_FLAG_BPART;
            else if(*fe.type=='B')
                fe.flag=DTREE_DT_FLAG_BCHAIN|DTREE_DT_FLAG_CHAIN;
            else if(*fe.type=='C')
                fe.flag=DTREE_DT_FLAG_CHAIN;
            else
                fe.flag=DTREE_DT_FLAG_NONE;
        }
        else
            fe.flag=DTREE_DT_FLAG_NONE;

        dtree_printd(DTREE_PRINT_INITDTREE,
                "LOAD: line dump: pattern: '%s' id: '%s':%zu type: '%s' flag: %ud cparam: '%s':%zu legacy: %d\nKVS",
                fe.pattern,fe.id,fe.id_len,fe.type,fe.flag,fe.cparam,fe.cparam_len,fe.legacy);
        for(i=0;i<fe.count;i++)
            dtree_printd(DTREE_PRINT_INITDTREE,",'%s':%zu='%s':%zu",fe.p[i].key,fe.p[i].key_len,fe.p[i].value,fe.p[i].val_len);
        dtree_printd(DTREE_PRINT_INITDTREE,"\n");
        
        //look for this entry
        kvd=(dclass_keyvalue*)dtree_get(&ids,fe.id,0);
        
        if(!kvd)
        {
            kvd=(dclass_keyvalue*)dtree_alloc_mem(h,sizeof(dclass_keyvalue));
            
            if(!kvd)
                goto lerror;
            
            //populate kvd
            kvd->id=dtree_alloc_string(h,fe.id,fe.id_len);
            
            if(dtree_add_entry(&ids,kvd->id,kvd,0,NULL)<0)
                goto lerror;
            
            dtree_printd(DTREE_PRINT_INITDTREE,"IDS: Successful add: '%s'\n",kvd->id);
        }
        
        //copy kvs
        if(fe.count && !kvd->size)
        {
            if(fe.count!=keys_size)
            {
                keys_size=0;
                keys=NULL;
            }
            
            if(keys)
            {
                for(i=0;i<fe.count;i++)
                {
                    for(j=0;j<keys_size;j++)
                    {
                        if(!strcasecmp(fe.p[i].key,keys[j]))
                            break;
                    }
                    
                    if(j>=keys_size)
                    {   
                        keys_size=0;
                        keys=NULL;
                        
                        break;
                    }
                }
            }
            
            if(!keys)
            {
                dtree_printd(DTREE_PRINT_INITDTREE,"LOAD: allocating new keys count: %d\n",fe.count);
                
                keys=(char**)dtree_alloc_mem(h,fe.count*sizeof(char*));
                
                if(!keys)
                    goto lerror;
                
                keys_size=fe.count;
                
                for(i=0;i<fe.count;i++)
                    keys[i]=dtree_alloc_string(h,fe.p[i].key,fe.p[i].key_len);
            }

            kvd->keys=(const char**)keys;

            kvd->size=fe.count;

            kvd->values=(const char**)dtree_alloc_mem(h,kvd->size*sizeof(char*));
            
            if(!kvd->values)
                goto lerror;
            
            for(i=0;i<kvd->size;i++)
            {
                if(fe.p[i].value)
                    kvd->values[i]=dtree_alloc_string(h,fe.p[i].value,fe.p[i].val_len);
            }
        }
        
        cparam=NULL;
        
        //lookup cparam
        if(fe.cparam && *fe.cparam)
        {
            dtree_printd(DTREE_PRINT_INITDTREE,"LOAD: cparam detected: '%s'\n",fe.cparam);
            
            cparam=(dclass_keyvalue*)dtree_get(&ids,fe.cparam,0);
            
            //add cparam
            if(!cparam)
            {
                cparam=(dclass_keyvalue*)dtree_alloc_mem(h,sizeof(dclass_keyvalue));
            
                if(!cparam)
                    goto lerror;

                //populate kvd
                cparam->id=dtree_alloc_string(h,fe.cparam,fe.cparam_len);
            
                if(dtree_add_entry(&ids,cparam->id,cparam,0,NULL)<0)
                    goto lerror;

                dtree_printd(DTREE_PRINT_INITDTREE,"IDS: Successful add: '%s'\n",cparam->id);
            }
        }
        
        //add the entry
        ret=dtree_add_entry(h,fe.pattern,kvd,fe.flag,cparam);
        
        if(ret<0)
            goto lerror;
        else if(ret>0)
            dtree_printd(DTREE_PRINT_INITDTREE,"LOAD: Successful add, nodes: %u, size: %lu\n",
                    h->node_count,h->size);
        
#if DTREE_PERF_WALKING
        if(!(lines%10))
        {
            total=high=low=0;
            for(i=0;i<5;i++)
            {
                clock_gettime(CLOCK_REALTIME,&startn);
                lret=dtree_print(h,NULL);
                clock_gettime(CLOCK_REALTIME,&endn);
                dtree_timersubn(&endn,&startn,&diffn);
                val=(diffn.tv_sec*1000*1000*1000)+diffn.tv_nsec;
                if(diffn.tv_nsec==0 && diffn.tv_sec==0)
                {
                    i--;
                    continue;
                }
                if(val>high)
                    high=val;
                if(!i || val<low)
                    low=val;
                total+=val;
            }
            total=(total-high-low)/3;
            printf("walk tree: %ld tokens %zu nodes time: %lds %ldms %ldus %ldns\n",
                    lret,h->node_count,total/(1000*1000*1000),total/1000000%1000,total/1000%1000,total%1000);
        }
#endif
    }
    
    dtree_free(&ids);
    fclose(f);
    
    return lines;
    
lerror:
                    
    dtree_free(h);
    dtree_free(&ids);
    fclose(f);
    
    return -1;
}

//parses a line into a file_entry
static void dclass_parse_fentry(char *buf,dtree_file_entry *fe)
{
    int count=0;
    char sep;
    char *p;
    char *t;
    char *key=NULL;
    size_t klen=0;
    size_t len;
    
    for(p=buf;*p;p++)
    {
        //start
        t=p;
        
        //end
        for(;*p;p++)
        {
            if(*p==';' || *p==',' || *p=='=' || *p=='\n')
            {
                if(*t=='\"' && *(p-1)=='\"')
                {
                    t++;
                    *(p-1)='\0';
                }
                else if(*t=='\"' && *p!='\n')
                    continue;
                
                break;
            }
        }
        
        sep=*p;
        *p='\0';
        
        len=p-t;
        
        if(!*(p-1))
            len--;
        
        if(len>=DTREE_DATA_BUFLEN)
            len=DTREE_DATA_BUFLEN-1;

        if(!count)
            fe->pattern=t;
        else if(count==1 && sep=='=')
        {
            fe->legacy=1;
            count=4;
            key=t;
            klen=len;
            continue;
        }
        else if(count==1)
        {
            fe->id=t;
            fe->id_len=len;
        }
        else if(count==2)
            fe->type=t;
        else if(count==3)
        {
            fe->cparam=t;
            fe->cparam_len=len;
        }
        else if(count>=4)
        {
            if(!key)
            {
                key=t;
                klen=len;
                
                if(sep==',')
                {
                    t=NULL;
                    len=0;
                }
                else
                    continue;
            }
            
            fe->p[fe->count].key=key;
            fe->p[fe->count].key_len=klen;
            fe->p[fe->count].value=t;
            fe->p[fe->count].val_len=len;
            
            fe->count++;
            
            key=NULL;
        }
        
        count++;
    }
}

//writes a dtree file
int dclass_write_file(const dclass_index *di,const char *outFile)
{
    int lines=0;
    unsigned int i;
    char buf[DTREE_DATA_BUFLEN];
    FILE *f;
    const dtree_dt_index *h=&di->dti;
    
    if(!(f=fopen(outFile,"w")))
    {
        fprintf(stderr,"DTREE: cannot open '%s'\n",outFile);
        return -1;
    }
    
    fputs("# dtree dump\n",f);
    
    if(h->sflags & DTREE_S_FLAG_PARTIAL)
        fputs("#$partial\n",f);
    
    if(h->sflags & DTREE_S_FLAG_REGEX)
        fputs("#$regex\n",f);
    
    if(h->sflags & DTREE_S_FLAG_DUPS)
        fputs("#$dups\n",f);
    
    //dump cache
    if(h->dc_cache[0])
    {
        fputs("#!",f);
        for(i=0;h->dc_cache[i] && i<DTREE_M_LOOKUP_CACHE;i++)
        {
            if(i && !(i%8))
                fputs("\n#!",f);
            else if(i)
                fputc(',',f);
            fputs(h->dc_cache[i],f);
        }
        fputc('\n',f);
    }
    
    for(i=0;h->head && i<DTREE_HASH_NCOUNT;i++)
        lines+=dclass_write_tree(h,DTREE_DT_GETPP(h,h->head->nodes[i]),buf,0,f);
    
    fclose(f);
    
    return lines;
}

static long dclass_write_tree(const dtree_dt_index *h,const dtree_dt_node *n,char *path,int depth,FILE *f)
{
    unsigned int i;
    long tot=0;
    packed_ptr pp;
    const dtree_dt_node *dupn;
    
    if(!n || depth>(DTREE_DATA_BUFLEN-1))
        return 0;
    
    path[depth]=n->data;
    path[depth+1]='\0';
    
    if(n->flags & DTREE_DT_FLAG_TOKEN)
    {
        dclass_write_node(n,path,f);
        
        tot++;
    }
    
    pp=n->dup;
    dupn=DTREE_DT_GETPP(h,pp);
    
    while(pp)
    {
        dclass_write_node(dupn,path,f);
        
        tot++;
        
        pp=dupn->dup;
        dupn=DTREE_DT_GETPP(h,pp);
    }
    
    for(i=0;i<DTREE_HASH_NCOUNT;i++)
    {
        if(n->nodes[i])
            tot+=dclass_write_tree(h,DTREE_DT_GETPP(h,n->nodes[i]),path,depth+1,f);
    }
    
    path[depth]='\0';
    
    return tot;
}

static void dclass_write_node(const dtree_dt_node *n,char *path,FILE *f)
{
    int i;
    dclass_keyvalue *s;
    
    s=(dclass_keyvalue*)n->payload;
    
    fputc('\"',f);
    
    fputs(path,f);
    
    fputs("\";\"",f);
    
    fputs(s->id,f);
    
    fputs("\";",f);

    if(n->flags & DTREE_DT_FLAG_STRONG)
        fputc('S',f);
    else if(n->flags & DTREE_DT_FLAG_WEAK)
        fputc('W',f);
    else if(n->flags & DTREE_DT_FLAG_BPART)
        fputc('P',f);
    else if(n->flags & DTREE_DT_FLAG_BCHAIN)
        fputc('B',f);
    else if(n->flags & DTREE_DT_FLAG_CHAIN)
        fputc('C',f);
    else
        fputc('N',f);
    
    fputc(';',f);
    
    if(n->cparam)
    {
        fputc('\"',f);
        fputs(((dclass_keyvalue*)n->cparam)->id,f);
        fputc('\"',f);
    }
    
    fputc(';',f);
    
    for(i=0;i<s->size;i++)
    {
        if(i)
            fputc(',',f);
        fputc('\"',f);
        fputs(s->keys[i],f);
        fputc('\"',f);
        if(s->values[i])
        {
            fputs("=\"",f);
            fputs(s->values[i],f);
            fputc('\"',f);
        }
    }
    
    fputc('\n',f);
}
