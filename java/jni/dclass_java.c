#include "dclass_client.h"
#include "dclass_java.h"

#if _WIN64
typedef long long plong;
#else
typedef long plong;
#endif

void Java_dclass_dClass_setindex(JNIEnv *env,jobject obj,dclass_index *dci)
{
    jclass cls;
    jfieldID fid=NULL;

    cls=(*env)->GetObjectClass(env,obj);

    if(cls)
        fid=(*env)->GetFieldID(env,cls,"dclass_index","J");

    if(fid)
        (*env)->SetLongField(env,obj,fid,(plong)dci);
}

JNIEXPORT jint JNICALL Java_dclass_dClass_init(JNIEnv *env,jobject obj,jstring jpath)
{
    jint ret;
    const char *path;
    dclass_index *dci;

    path=(*env)->GetStringUTFChars(env,jpath,NULL);

    if(!path)
        return -1;

    dci=calloc(1,sizeof(dclass_index));

    if(!dci)
    {
        (*env)->ReleaseStringUTFChars(env,jpath,path);
        return -1;
    }

    ret=dclass_load_file(dci,path);

    if(ret<=0)
        ret=devicemap_load_resources(dci,path);

    (*env)->ReleaseStringUTFChars(env,jpath,path);

    Java_dclass_dClass_setindex(env,obj,dci);

    return ret;
}

JNIEXPORT jint JNICALL Java_dclass_dClass_write(JNIEnv *env,jobject obj,jlong ptr,jstring jpath)
{
    jint ret;
    const char *path;
    dclass_index *dci;

    dci=(dclass_index*)(plong)ptr;

    path=(*env)->GetStringUTFChars(env,jpath,NULL);

    if(!path)
        return -1;

    ret=dclass_write_file(dci,path);

    (*env)->ReleaseStringUTFChars(env,jpath,path);

    return ret;
}

JNIEXPORT jlong JNICALL Java_dclass_dClass_walk(JNIEnv *env,jobject obj,jlong ptr)
{
    dclass_index *dci;

    dci=(dclass_index*)(plong)ptr;

    return (plong)dtree_print(&(dci->dti),&dclass_get_id);
}

JNIEXPORT jstring JNICALL Java_dclass_dClass_statversion(JNIEnv *env,jclass cls)
{
    return (*env)->NewStringUTF(env,DCLASS_VERSION);
}

JNIEXPORT jstring JNICALL Java_dclass_dClass_stataddressing(JNIEnv *env,jclass cls)
{
    char buf[10];

    *buf='\0';

    sprintf(buf,"%s%zu",DTREE_DT_PTR_TYPE,DTREE_DT_PTR_SIZE);

    return (*env)->NewStringUTF(env,buf);
}

JNIEXPORT jlong JNICALL Java_dclass_dClass_statnodes(JNIEnv *env,jobject obj,jlong ptr)
{
    dclass_index *dci;

    dci=(dclass_index*)(plong)ptr;

    return (jlong)dci->dti.node_count;
}

JNIEXPORT jlong JNICALL Java_dclass_dClass_statnodesize(JNIEnv *env,jclass cls)
{
    return (jlong)sizeof(dtree_dt_node);
}

JNIEXPORT jlong JNICALL Java_dclass_dClass_statmemory(JNIEnv *env,jobject obj,jlong ptr)
{
    dclass_index *dci;

    dci=(dclass_index*)(plong)ptr;

    return (jlong)dci->dti.size;
}

JNIEXPORT jstring JNICALL Java_dclass_dClass_comment(JNIEnv *env,jobject obj,jlong ptr)
{
    dclass_index *dci;

    dci=(dclass_index*)(plong)ptr;

    if(dci->dti.comment)
        return (*env)->NewStringUTF(env,dci->dti.comment);
    else
        return (*env)->NewStringUTF(env,"");
}

JNIEXPORT jlong JNICALL Java_dclass_dClass_classify(JNIEnv *env,jobject obj,jlong ptr,jstring js)
{
    const char *s;
    dclass_index *dci;
    const dclass_keyvalue *kv;

    dci=(dclass_index*)(plong)ptr;

    s=(*env)->GetStringUTFChars(env,js,NULL);

    kv=dclass_classify(dci,s);

    (*env)->ReleaseStringUTFChars(env,js,s);

    return (plong)kv;
}

JNIEXPORT jlong JNICALL Java_dclass_dClass_get(JNIEnv *env,jobject obj,jlong ptr,jstring js)
{
    const char *s;
    dclass_index *dci;
    const dclass_keyvalue *kv;

    dci=(dclass_index*)(plong)ptr;

    s=(*env)->GetStringUTFChars(env,js,NULL);

    kv=dclass_get(dci,s);

    (*env)->ReleaseStringUTFChars(env,js,s);

    return (plong)kv;
}

JNIEXPORT jint JNICALL Java_dclass_dClass_kvlength(JNIEnv *env,jobject obj,jlong jkv)
{
    const dclass_keyvalue *kv;

    kv=(dclass_keyvalue*)(plong)jkv;

    return (jint)kv->size;
}

JNIEXPORT jstring JNICALL Java_dclass_dClass_kvgetid(JNIEnv *env,jobject obj,jlong jkv)
{
    const dclass_keyvalue *kv;

    kv=(dclass_keyvalue*)(plong)jkv;

    return (*env)->NewStringUTF(env,kv->id);
}

JNIEXPORT jstring JNICALL Java_dclass_dClass_kvgetkey(JNIEnv *env,jobject obj,jlong jkv,jint pos)
{
    const dclass_keyvalue *kv;

    kv=(dclass_keyvalue*)(plong)jkv;

    if(pos>=0 && pos<kv->size)
        return (*env)->NewStringUTF(env,kv->keys[pos]);
    else
        return (*env)->NewStringUTF(env,DTREE_M_SERROR);
}

JNIEXPORT jstring JNICALL Java_dclass_dClass_kvgetvalue(JNIEnv *env,jobject obj,jlong jkv,jint pos)
{
    const dclass_keyvalue *kv;

    kv=(dclass_keyvalue*)(plong)jkv;

    if(pos>=0 && pos<kv->size)
        return (*env)->NewStringUTF(env,kv->values[pos]);
    else
        return (*env)->NewStringUTF(env,DTREE_M_SERROR);
}

JNIEXPORT void JNICALL Java_dclass_dClass_free(JNIEnv *env,jobject obj,jlong ptr)
{
    dclass_index *dci;

    dci=(dclass_index*)(plong)ptr;

    Java_dclass_dClass_setindex(env,obj,NULL);

    dclass_free(dci);

    free(dci);
}
