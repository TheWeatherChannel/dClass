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


//structs for parsing dtree file
typedef struct
{
    char *key;
    char *value;
    
    size_t key_len;
    size_t val_len;
}
dtree_kv_pair;

typedef struct
{
    char *pattern;
    char *id;
    char *cparam;
    char *type;
    
    size_t pattern_len;
    size_t id_len;
    size_t cparam_len;
    
    int count;
    
    dtree_flag_f flag;

    unsigned int pos;
    unsigned int rank;
    signed int dir;
    
    dtree_kv_pair p[DTREE_DATA_MKEYS];
}
dtree_file_entry;


static void dclass_parse_fentry(char*,dtree_file_entry*,int);
static long dclass_write_tree(const dtree_dt_index*,const dtree_dt_node*,char*,int,FILE*);
static void dclass_write_node(const dtree_dt_node*,char*,FILE*);

extern unsigned int dtree_hash_char(char);


#if DTREE_PERF_WALKING
extern void dtree_timersubn(struct timespec*,struct timespec*,struct timespec*);
extern int dtree_gettime(struct timespec*);
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
    dtree_dt_add_entry entry;
    
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

                            memset(&entry,0,sizeof(dtree_dt_add_entry));

                            entry.data=kvd;

                            if(dtree_add_entry(&ids,kvd->id,&entry)<0)
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
                    else if(!strncasecmp(p,"notrim",6))
                    {
                        h->sflags |= DTREE_S_FLAG_NOTRIM;
                        dtree_printd(DTREE_PRINT_INITDTREE,"INIT FORCE: notrim\n");
                    }
                    break;
                //kv error id
                case '@':
                    p++;
                    for(s=p;*p;p++)
                    {
                        if(*p=='\n')
                            break;
                    }
                    *p='\0';
                    di->error.id=dtree_alloc_string(h,s,p-s+1);
                //comment
                default:
                    if(!h->comment)
                    {
                        if(*p==' ')
                            p++;
                        for(s=p;*p;p++)
                        {
                            if(*p=='\n')
                                break;
                        }
                        *p='\0';
                        h->comment=dtree_alloc_mem(h,(p-s+1)*sizeof(char));
                        if(h->comment)
                            strncpy(h->comment,s,p-s);
                    }
                    break;
            }          
            continue;
        }
        
        memset(&fe,0,sizeof(fe));
        
        //parse the line
        dclass_parse_fentry(buf,&fe,h->sflags & DTREE_S_FLAG_NOTRIM);
        
        if(!fe.id)
        {
            dtree_printd(DTREE_PRINT_INITDTREE,"LOAD: bad line detected: '%s':%d\n",buf,lines);
            continue;
        }
                
        if(fe.type)
        {
            for(s=fe.type;*s;s++)
            {
                if(*s==':')
                {
                    s++;
                    fe.pos=strtol(s,NULL,10);
                    break;
                }
                else if(fe.flag)
                    continue;
                else if(*s=='S')
                    fe.flag=DTREE_DT_FLAG_STRONG;
                else if(*s=='W')
                {
                    fe.flag=DTREE_DT_FLAG_WEAK;
                    if(*(s+1)>='0' && *(s+1)<='9')
                        fe.rank=strtol(s+1,NULL,10);
                }
                else if(*s=='B')
                    fe.flag=DTREE_DT_FLAG_BCHAIN;
                else if(*s=='C')
                {
                    fe.flag=DTREE_DT_FLAG_CHAIN;
                    if(*(s+1)>='0' && *(s+1)<='9')
                        fe.rank=strtol(s+1,NULL,10);
                }
                else if(*s=='N')
                    fe.flag=DTREE_DT_FLAG_NONE;
            }
        }
    
        if(!fe.flag)
            fe.flag=DTREE_DT_FLAG_NONE;

        if(fe.cparam)
        {
            for(s=fe.cparam;*s;s++)
            {
                if(*s==':')
                {
                    *s='\0';

                    fe.cparam_len=s-fe.cparam;

                    fe.dir=strtol(s+1,NULL,10);
                }
            }
        }

        if(fe.pos>DTREE_S_BE_POS)
            fe.pos=DTREE_S_MAX_POS;
        if(fe.rank>DTREE_S_MAX_RANK)
            fe.rank=DTREE_S_MAX_RANK;
        if(fe.dir<DTREE_S_MIN_DIR)
            fe.dir=DTREE_S_MIN_DIR;
        else if(fe.dir>DTREE_S_MAX_DIR)
            fe.dir=DTREE_S_MAX_DIR;

        if(*fe.pattern==DTREE_PATTERN_BEGIN)
        {
            fe.pos=1;
            fe.pattern++;
            fe.pattern_len--;
        }

        if(*(fe.pattern+fe.pattern_len-1)==DTREE_PATTERN_END && *(fe.pattern+fe.pattern_len-2)!=DTREE_PATTERN_ESCAPE)
        {
            if(fe.pos==1)
                fe.pos=DTREE_S_BE_POS;
            else
                fe.pos=DTREE_S_MAX_POS;

            *(fe.pattern+fe.pattern_len-1)='\0';
            fe.pattern_len--;
        }

        dtree_printd(DTREE_PRINT_INITDTREE,
                "LOAD: line dump: pattern: '%s':%zu id: '%s':%zu type: '%s' flag: %ud cparam: '%s':%zu prd: %d,%d,%d\nKVS",
                fe.pattern,fe.pattern_len,fe.id,fe.id_len,fe.type,fe.flag,fe.cparam,fe.cparam_len,fe.pos,fe.rank,fe.dir);
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

            memset(&entry,0,sizeof(dtree_dt_add_entry));

            entry.data=kvd;
            
            if(dtree_add_entry(&ids,kvd->id,&entry)<0)
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
            dtree_printd(DTREE_PRINT_INITDTREE,"LOAD: cparam detected: '%s':%d\n",fe.cparam,fe.dir);
            
            cparam=(dclass_keyvalue*)dtree_get(&ids,fe.cparam,0);
            
            //add cparam
            if(!cparam)
            {
                cparam=(dclass_keyvalue*)dtree_alloc_mem(h,sizeof(dclass_keyvalue));
            
                if(!cparam)
                    goto lerror;

                //populate kvd
                cparam->id=dtree_alloc_string(h,fe.cparam,fe.cparam_len);

                memset(&entry,0,sizeof(dtree_dt_add_entry));

                entry.data=cparam;
            
                if(dtree_add_entry(&ids,cparam->id,&entry)<0)
                    goto lerror;

                dtree_printd(DTREE_PRINT_INITDTREE,"IDS: Successful add: '%s'\n",cparam->id);
            }
        }
        
        memset(&entry,0,sizeof(dtree_dt_add_entry));

        entry.data=kvd;
        entry.flags=fe.flag;
        entry.param=cparam;
        entry.pos=fe.pos;
        entry.rank=fe.rank;
        entry.dir=fe.dir;

        //add the entry
        ret=dtree_add_entry(h,fe.pattern,&entry);
        
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
                dtree_gettime(&startn);
                lret=dtree_print(h,NULL);
                dtree_gettime(&endn);
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

    if(!di->error.id || !strcmp(DTREE_M_SERROR,di->error.id)) {
        if(DTREE_M_LOOKUP_CACHE && h->dc_cache[0])
            di->error.id=h->dc_cache[0];
    }
    
    return lines;
    
lerror:
                    
    dtree_free(h);
    dtree_free(&ids);
    fclose(f);
    
    return -1;
}

//parses a line into a file_entry
static void dclass_parse_fentry(char *buf,dtree_file_entry *fe,int notrim)
{
    int count=0;
    char sep;
    char *p;
    char *t;
    char *e;
    char *key=NULL;
    size_t klen=0;
    size_t len=0;
    
    for(p=buf;*p;p++)
    {
        //start
        e=t=p;
        
        //end
        for(;*p;p++)
        {
            e=p;
            len=p-t;

            if((*p==';' && count<4) || ((*p==',' || *p=='=') && count>=4) || *p=='\n')
            {
                if(!notrim)
                {
                    while((*t==' ' || *t=='\t') && t<p)
                        t++;
                    while((*(p-1)==' ' || *(p-1)=='\t') && p>t)
                        p--;

                    len=p-t;
                }
                if(*t==DTREE_PATTERN_DQUOTE && p>(t+1) && *(p-1)==DTREE_PATTERN_DQUOTE && *(p-2)!=DTREE_PATTERN_ESCAPE)
                {
                    t++;
                    *(p-1)='\0';
                    len-=2;
                }
                else if(*t==DTREE_PATTERN_DQUOTE && *p!='\n')
                {
                    if(!notrim)
                        p=e;
                    continue;
                }
                
                break;
            }
        }
        
        sep=*p;

        if(!notrim)
            sep=*e;

        *p='\0';

        p=e;

        if(strlen(t)!=len)
            dtree_printd(DTREE_PRINT_INITDTREE,"LOAD: ERROR: '%s' != %zu\n",t,len);

        if(!count)
        {
            fe->pattern=t;
            fe->pattern_len=len;
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

            if(fe->count>=DTREE_DATA_MKEYS)
                break;
            
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
    
    if(h->comment)
    {
        fputs("# ",f);
        fputs(h->comment,f);
        fputc('\n',f);
    }
    else
        fputs("# dtree dump\n",f);
    
    if(h->sflags & DTREE_S_FLAG_PARTIAL)
        fputs("#$partial\n",f);
    
    if(h->sflags & DTREE_S_FLAG_REGEX)
        fputs("#$regex\n",f);
    
    if(h->sflags & DTREE_S_FLAG_DUPS)
        fputs("#$dups\n",f);
    
    if(di->error.id && strcmp(DTREE_M_SERROR,di->error.id)) {
        fputs("#@",f);
        fputs(di->error.id,f);
        fputc('\n',f);
    }

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
    
    if(!n || !n->curr || depth>(DTREE_DATA_BUFLEN-1))
        return 0;
    
    if(h->sflags & DTREE_S_FLAG_REGEX)
    {
        switch(n->data)
        {
            case DTREE_PATTERN_ANY:
                pp=n->prev;
                dupn=DTREE_DT_GETPP(h,pp);
                i=dtree_hash_char(n->data);
                if(i==DTREE_HASH_ANY || (pp && dupn->nodes[i]!=n->curr))
                    break;
            case DTREE_PATTERN_OPTIONAL:
            case DTREE_PATTERN_SET_S:
            case DTREE_PATTERN_SET_E:
            case DTREE_PATTERN_GROUP_S:
            case DTREE_PATTERN_GROUP_E:
            case DTREE_PATTERN_ESCAPE:
            case DTREE_PATTERN_DQUOTE:
                path[depth]=DTREE_PATTERN_ESCAPE;
                depth++;
        }
    }

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

    if(path[depth-1]==DTREE_PATTERN_ESCAPE)
        path[depth-1]='\0';
    
    return tot;
}

static void dclass_write_node(const dtree_dt_node *n,char *path,FILE *f)
{
    int i;
    dclass_keyvalue *s;
    
    s=(dclass_keyvalue*)n->payload;
    
    fputc(DTREE_PATTERN_DQUOTE,f);
    
    fputs(path,f);
    
    fputc(DTREE_PATTERN_DQUOTE,f);
    fputc(';',f);
    fputc(DTREE_PATTERN_DQUOTE,f);
    
    fputs(s->id,f);
    
    fputc(DTREE_PATTERN_DQUOTE,f);
    fputc(';',f);

    if(n->flags & DTREE_DT_FLAG_STRONG)
        fputc('S',f);
    else if(n->flags & DTREE_DT_FLAG_WEAK)
    {
        fputc('W',f);
        if(n->rank)
            fprintf(f,"%hd",(short int)n->rank);
    }
    else if(n->flags & DTREE_DT_FLAG_BCHAIN)
        fputc('B',f);
    else if(n->flags & DTREE_DT_FLAG_CHAIN)
    {
        fputc('C',f);
        if(n->rank)
            fprintf(f,"%hd",(short int)n->rank);
    }
    else
        fputc('N',f);
    
    if(n->pos)
        fprintf(f,":%hd",(short int)n->pos);

    fputc(';',f);
    
    if(n->cparam)
    {
        fputc(DTREE_PATTERN_DQUOTE,f);
        fputs(((dclass_keyvalue*)n->cparam)->id,f);
        if(n->dir)
            fprintf(f,":%d",n->dir);
        fputc(DTREE_PATTERN_DQUOTE,f);
    }
    
    fputc(';',f);
    
    for(i=0;i<s->size;i++)
    {
        if(i)
            fputc(',',f);
        fputc(DTREE_PATTERN_DQUOTE,f);
        fputs(s->keys[i],f);
        fputc(DTREE_PATTERN_DQUOTE,f);
        if(s->values[i])
        {
            fputc('=',f);
            fputc(DTREE_PATTERN_DQUOTE,f);
            fputs(s->values[i],f);
            fputc(DTREE_PATTERN_DQUOTE,f);
        }
    }
    
    fputc('\n',f);
}
