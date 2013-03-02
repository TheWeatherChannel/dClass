#include "dclass_client.h"
#include "dclass_java.h"

void Java_dclass_dClass_setindex(JNIEnv *env,jobject obj,dclass_index *dci)
{
    jclass cls;
    jfieldID fid=NULL;

    cls=(*env)->GetObjectClass(env,obj);

    if(cls)
        fid=(*env)->GetFieldID(env,cls,"dclass_index","J");

    if(fid)
        (*env)->SetLongField(env,obj,fid,(long)dci);
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
        return -1;

    ret=dclass_load_file(dci,path);

    if(ret<=0)
        ret=openddr_load_resources(dci,path);

    (*env)->ReleaseStringUTFChars(env,jpath,path);

    Java_dclass_dClass_setindex(env,obj,dci);

    return ret;
}

JNIEXPORT jlong JNICALL Java_dclass_dClass_classify(JNIEnv *env,jobject obj,jlong ptr,jstring js)
{
    const char *s;
    dclass_index *dci;
    const dclass_keyvalue *kv;

    dci=(dclass_index*)(long)ptr;

    s=(*env)->GetStringUTFChars(env,js,NULL);

    kv=dclass_classify(dci,s);

    (*env)->ReleaseStringUTFChars(env,js,s);

    return (long)kv;
}

JNIEXPORT jlong JNICALL Java_dclass_dClass_get(JNIEnv *env,jobject obj,jlong ptr,jstring js)
{
    const char *s;
    dclass_index *dci;
    const dclass_keyvalue *kv;

    dci=(dclass_index*)(long)ptr;

    s=(*env)->GetStringUTFChars(env,js,NULL);

    kv=dclass_get(dci,s);

    (*env)->ReleaseStringUTFChars(env,js,s);

    return (long)kv;
}

JNIEXPORT jint JNICALL Java_dclass_dClass_kvlength(JNIEnv *env,jobject obj,jlong jkv)
{
    const dclass_keyvalue *kv;

    kv=(dclass_keyvalue*)(long)jkv;

    return (jint)kv->size;
}

JNIEXPORT jstring JNICALL Java_dclass_dClass_kvgetid(JNIEnv *env,jobject obj,jlong jkv)
{
    jstring ret;
    const dclass_keyvalue *kv;

    kv=(dclass_keyvalue*)(long)jkv;

    ret=(*env)->NewStringUTF(env,kv->id);

    return ret;
}

JNIEXPORT jstring JNICALL Java_dclass_dClass_kvgetkey(JNIEnv *env,jobject obj,jlong jkv,jint pos)
{
    jstring ret;
    const dclass_keyvalue *kv;

    kv=(dclass_keyvalue*)(long)jkv;

    if(pos>=0 && pos<kv->size)
        ret=(*env)->NewStringUTF(env,kv->keys[pos]);
    else
        ret=(*env)->NewStringUTF(env,DTREE_M_SERROR);

    return ret;
}

JNIEXPORT jstring JNICALL Java_dclass_dClass_kvgetvalue(JNIEnv *env,jobject obj,jlong jkv,jint pos)
{
    jstring ret;
    const dclass_keyvalue *kv;

    kv=(dclass_keyvalue*)(long)jkv;

    if(pos>=0 && pos<kv->size)
        ret=(*env)->NewStringUTF(env,kv->values[pos]);
    else
        ret=(*env)->NewStringUTF(env,DTREE_M_SERROR);

    return ret;
}

JNIEXPORT void JNICALL Java_dclass_dClass_free(JNIEnv *env,jobject obj,jlong ptr)
{
    dclass_index *dci;

    dci=(dclass_index*)(long)ptr;

    Java_dclass_dClass_setindex(env,obj,NULL);

    dclass_free(dci);

    free(dci);
}
