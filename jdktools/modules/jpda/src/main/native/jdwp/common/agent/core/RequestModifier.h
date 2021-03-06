/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/**
 * @author Pavel N. Vyssotski
 */

/**
 * @file
 * RequestModifier.h
 *
 */

#ifndef _REQUEST_MODIFIER_H_
#define _REQUEST_MODIFIER_H_

#include "AgentBase.h"

namespace jdwp {

    /**
     * The event description structure used in <code>RequestManager</code>.
     */
    struct EventInfo {

        /**
         * The JDWP kind of the request event.
         */
        jdwpEventKind kind;

        /**
         * The Java thread where the event occurred.
         */
        jthread thread;

        /**
         * The Java class in which the method event occurred.
         */
        jclass cls;

        /**
         * The signature of the Java class in which the method event occurred.
         */
        char* signature;

        /**
         * The method ID where the event occurred.
         */
        jmethodID method;

        /**
         * The Java location where the event occurred.
         */
        jlocation location;

        /**
         * The field ID accessed or modified on the corresponding
         * event, such as <code>FieldAccess</code> and 
         * <code>FieldModification</code>.
         */
        jfieldID field;

        /**
         * The Java object which field was accessed or modified on the corresponding
         * event, such as <code>FieldAccess</code> and 
         * <code>FieldModification</code>.
         */
        jobject instance;

        /**
         * The aux class corresponding to the exception in the Exception event or
         * to the field reference type in <code>FieldAccess</code> and 
         * <code>FieldModification</code> events.
         */
        jclass auxClass;

        /**
         * The flag indicating that the exception thrown in the Exception event
         * was caught.
         */
        bool caught;
    };

    /**
     * The base class for event request modifiers used to filter the events
     * generated by the target VM.
     */
    class RequestModifier : public AgentBase {

    public:

        /**
         * A constructor.
         *
         * @param kind - the JDWP request modifier kind
         */
        RequestModifier(jdwpRequestModifier kind) : m_kind(kind) {}

        /**
         * A destructor.
         */
        virtual ~RequestModifier() {};

        /**
         * Applies filtering for the given event.
         *
         * @param jni    - the JNI interface pointer
         * @param eInfo  - the request-event information
         *
         * @return <code>TRUE</code>.
         */
        virtual bool Apply(JNIEnv* jni, EventInfo &eInfo) throw() {
            return true;
        }

        /**
         * Gets the JDWP request modifier kind.
         *
         * @return The JDWP request modifier kind.
         */
        jdwpRequestModifier GetKind() const throw() { return m_kind; }

    protected:

        bool MatchPattern(const char *signature, const char *pattern)
            const throw();

        jdwpRequestModifier m_kind;

    };

    /**
     * The class implements the Count modifier enabling the requested events
     * to be reported only once after specified number of occurrences.
     * 
     */
    class CountModifier : public RequestModifier {

    public:

        /**
         * A constructor.
         *
         * @param n - the initial count value
         */
        CountModifier(jint n) :
            RequestModifier(JDWP_MODIFIER_COUNT),
            m_count(n)
        {}

        /**
         * Gets the current count value.
         *
         * @return The current value of the event counter.
         */
        jint GetCount() const throw() { return m_count; }

        /**
         * Applies count filtering for the given event.
         *
         * @param jni    - the JNI interface pointer
         * @param eInfo  - the request-event information
         *
         * @return Returns <code>TRUE</code> if count is zero, otherwise 
         * <code>FALSE</code>.
         */
        bool Apply(JNIEnv* jni, EventInfo &eInfo) throw()
        {
            if (m_count > 0) {
                m_count--;
                if (m_count == 0) {
                    return true;
                }
            }
            return false;
        }

    private:

        jint m_count;

    };

    /**
     * The class implements the Conditional modifier enabling the requested events
     * to be reported depending on the specified expression. Currently, not 
     * implemented.
     */
    class ConditionalModifier : public RequestModifier {

    public:

        /**
         * A constructor.
         *
         * @param id - the expression ID
         */
        ConditionalModifier(jint id) :
            RequestModifier(JDWP_MODIFIER_CONDITIONAL),
            m_exprID(id)
        {}

        /**
         * Gets the expression ID.
         *
         * @return The expression ID.
         */
        jint GetExprID() const throw() { return m_exprID; }

        /**
         * Applies filtering by the expression result for the given event.
         *
         * @param jni    - the JNI interface pointer
         * @param eInfo  - the request-event information
         *
         * @return <code>TRUE</code> (not implemented).
         */
        bool Apply(JNIEnv* jni, EventInfo &eInfo) throw() {
            return true;
        }

    private:

        jint m_exprID;

    };

    /**
     * The class implements the <code>ThreadOnly</code> modifier enabling 
     * the requested events to be reported only for the specified thread.
     */
    class ThreadOnlyModifier : public RequestModifier {

    public:

        /**
         * A constructor.
         *
         * @param jni     - the JNI interface pointer
         * @param thread  - the Java thread
         *
         * @throws OutOfMemoryException.
         */
        ThreadOnlyModifier(JNIEnv *jni, jthread thread) throw(AgentException) :
            RequestModifier(JDWP_MODIFIER_THREAD_ONLY)
        {
            m_thread = jni->NewGlobalRef(thread);
            if (m_thread == 0) {
                throw OutOfMemoryException();
            }
        }

        /**
         * A destructor.
         */
        ~ThreadOnlyModifier() {
            GetJniEnv()->DeleteGlobalRef(m_thread);
        }

        /**
         * Gets the Java thread.
         *
         * @return The Java thread.
         */
        jthread GetThread() const throw() {
            return m_thread;
        }

        /**
         * Applies filtering by belonging to the only thread for the given event.
         *
         * @param jni    - the JNI interface pointer
         * @param eInfo  - the request-event information
         *
         * @return Returns <code>TRUE</code>, if the event occurs in the 
         *         thread of interest.
         */
        bool Apply(JNIEnv* jni, EventInfo &eInfo) throw() 
        {
            JDWP_ASSERT(eInfo.thread != 0);
            return (JNI_TRUE == jni->IsSameObject(eInfo.thread, m_thread));
        }

    private:

        jthread m_thread;

    };

    /**
     * The class implements the <code>ClassOnly</code> modifier enabling the 
     * requested events to be reported only for a specified or derived class.
     */
    class ClassOnlyModifier : public RequestModifier {

    public:

        /**
         * A constructor.
         *
         * @param jni - the JNI interface pointer
         * @param cls - the Java class
         *
         * @throws OutOfMemoryException.
         */
        ClassOnlyModifier(JNIEnv *jni, jclass cls) throw(AgentException) :
            RequestModifier(JDWP_MODIFIER_CLASS_ONLY)
        {
            m_class = static_cast<jclass>(jni->NewGlobalRef(cls));
            if (m_class == 0) {
                throw OutOfMemoryException();
            }
        }

        /**
         * A destructor.
         */
        ~ClassOnlyModifier() {
            GetJniEnv()->DeleteGlobalRef(m_class);
        }

        /**
         * Gets the Java class.
         *
         * @return The Java class.
         */
        jclass GetClass() const throw() {
            return m_class;
        }

        /**
         * Applies filtering by belonging to the only class for the given event.
         *
         * @param jni    - the JNI interface pointer
         * @param eInfo  - the request-event information
         *
         * @return Returns <code>TRUE</code>, if event occurs in the class of interest.
         */
        bool Apply(JNIEnv* jni, EventInfo &eInfo) throw()
        {
            JDWP_ASSERT(eInfo.cls != 0);
            return (JNI_TRUE == jni->IsAssignableFrom(eInfo.cls, m_class));
        }

    private:

        jclass m_class;

    };

    /**
     * The class implements the <code>ClassMatch</code> modifier enabling the 
     * requested events to be reported only for the classes with the name 
     * corresponding to the given pattern.
     */
    class ClassMatchModifier : public RequestModifier {

    public:

        /**
         * A constructor.
         *
         * @param str - the class match pattern
         */
        ClassMatchModifier(char* str)
            : RequestModifier(JDWP_MODIFIER_CLASS_MATCH)
            , m_pattern(str)
        {}

        /**
         * A destructor.
         */
        ~ClassMatchModifier() {
            GetMemoryManager().Free(m_pattern JDWP_FILE_LINE);
        }

        /**
         * Gets the class-match pattern.
         *
         * @return Zero-terminated string.
         */
        const char* GetPattern() const throw() {
            return m_pattern;
        }

        /**
         * Applies the class match filtering for the given event.
         *
         * @param jni    - the JNI interface pointer
         * @param eInfo  - the request-event information
         *
         * @return Returns <code>TRUE</code>, if the class signature matches the given pattern.
         */
        bool Apply(JNIEnv* jni, EventInfo &eInfo) throw()
        {
            JDWP_ASSERT(eInfo.signature != 0);
            return (MatchPattern(eInfo.signature, m_pattern));
        }

    private:

        char* m_pattern;

    };

    /**
     * The class implements the <code>ClassExclude</code> modifier enabling the 
     * requested events to be reported only for the classes with the name not 
     * corresponding to the given pattern.
     */
    class ClassExcludeModifier : public RequestModifier {

    public:

        /**
         * A constructor.
         *
         * @param str - the class exclude pattern
         */
        ClassExcludeModifier(char* str) :
            RequestModifier(JDWP_MODIFIER_CLASS_EXCLUDE),
            m_pattern(str)
        {}

        /**
         * A destructor.
         */
        ~ClassExcludeModifier() {
            GetMemoryManager().Free(m_pattern JDWP_FILE_LINE);
        }

        /**
         * Gets the class exclude pattern.
         *
         * @return Zero-terminated string.
         */
        const char* GetPattern() const throw() {
            return m_pattern;
        }

        /**
         * Applies the class exclude filtering for the given event.
         *
         * @param jni    - the JNI interface pointer
         * @param eInfo  - the request event information
         *
         * @return Returns <code>TRUE</code>, if the class signature does not match 
         * the given pattern.
         */
        bool Apply(JNIEnv* jni, EventInfo &eInfo) throw()
        {
            JDWP_ASSERT(eInfo.signature != 0);
            return (!MatchPattern(eInfo.signature, m_pattern));
        }

    private:

        char* m_pattern;

    };

    /**
     * The class implements the <code>LocationOnly</code> modifier enabling 
     * the requested events to be reported only at the specified location.
     */
    class LocationOnlyModifier : public RequestModifier {

    public:

        /**
         * A constructor.
         *
         * @param jni     - the JNI interface pointer
         * @param cls     - the Java class
         * @param method  - the class method
         * @param loc     - the Java location
         *
         * @throws OutOfMemoryException.
         */
        LocationOnlyModifier(JNIEnv *jni, jclass cls, jmethodID method,
                jlocation loc) throw(AgentException) :
            RequestModifier(JDWP_MODIFIER_LOCATION_ONLY),
            m_method(method),
            m_location(loc)
        {
            m_class = static_cast<jclass>(jni->NewGlobalRef(cls));
            if (m_class == 0) {
                throw OutOfMemoryException();
            }
        }

        /**
         * A destructor.
         */
        ~LocationOnlyModifier() {
            GetJniEnv()->DeleteGlobalRef(m_class);
        }

        /**
         * Gets the Java class.
         *
         * @return The Java class.
         */
        jclass GetClass() const throw() {
            return m_class;
        }

        /**
         * Gets the Java class method ID.
         *
         * @return The method ID.
         */
        jmethodID GetMethod() const throw() { 
            return m_method;
        }

        /**
         * Gets the Java location.
         *
         * @return The Java location.
         */
        jlocation GetLocation() const throw() {
            return m_location;
        }

        /**
         * Applies filtering by belonging to the given location for the given event.
         *
         * @param jni - the JNI interface pointer
         * @param eInfo - the request-event information
         *
         * @return Returns <code>TRUE</code>, if event location is equal to the given one.
         */
        bool Apply(JNIEnv* jni, EventInfo &eInfo) throw()
        {
            JDWP_ASSERT(eInfo.cls != 0);
            return (eInfo.method == m_method && eInfo.location == m_location &&
                JNI_TRUE == jni->IsSameObject(eInfo.cls, m_class));
        }

    private:

        jclass m_class;
        jmethodID m_method;
        jlocation m_location;

    };

    /**
     * The class implements the <code>ExceptionOnly</code> modifier enabling the 
     * requested events caused by exception to be reported only for a specified 
     * exception reference type or for all exceptions if the specified type is null.
     */
    class ExceptionOnlyModifier : public RequestModifier {

    public:

        /**
         * A constructor.
         *
         * @param jni      - the JNI interface pointer
         * @param cls      - the Java class
         * @param caught   - the caught exception 
         * @param uncaught - the uncaught exception 
         *
         * @throws <code>OutOfMemoryException</code>.
         */
        ExceptionOnlyModifier(JNIEnv *jni, jclass cls, bool caught,
                bool uncaught) throw(AgentException) :
            RequestModifier(JDWP_MODIFIER_EXCEPTION_ONLY),
            m_caught(caught),
            m_uncaught(uncaught)
        {
            if (cls == 0) {
                m_class = 0;
            } else {
                m_class = static_cast<jclass>(jni->NewGlobalRef(cls));
                if (m_class == 0) {
                    throw OutOfMemoryException();
                }
            }
        }

        /**
         * A destructor.
         */
        ~ExceptionOnlyModifier() {
            GetJniEnv()->DeleteGlobalRef(m_class);
        }

        /**
         * Gets the Java class.
         *
         * @return The Java class.
         */
        jclass GetClass() const throw() {
            return m_class;
        }

        /**
         * Tells whether exception is caught.
         *
         * @return Boolean.
         */
        bool IsCaught() const throw() {
            return m_caught;
        }

        /**
         * Tells whether the exception is uncaught.
         *
         * @return Boolean.
         */
        bool IsUncaught() const throw() {
            return m_uncaught;
        }

        /**
         * Applies filtering by exception to the given location for the given event.
         *
         * @param jni    - the JNI interface pointer
         * @param eInfo  - the request-event information
         *
         * @return Returns <code>TRUE</code>, if the event location is equal to the given one.
         */
        bool Apply(JNIEnv* jni, EventInfo &eInfo) throw()
        {
            return ((eInfo.caught ? m_caught : m_uncaught) &&
                (m_class == 0 || (eInfo.cls != 0 &&
                    JNI_TRUE == jni->IsAssignableFrom(eInfo.auxClass, m_class))));
        }

    private:

        jclass m_class;
        bool m_caught;
        bool m_uncaught;

    };

    /**
     * The class implements the <code>FieldOnly</code> modifier enabling the 
     * requested field to access/modify events to be reported only for a 
     * specified field in the given class.
     */
    class FieldOnlyModifier : public RequestModifier {

    public:

        /**
         * A constructor.
         *
         * @param jni   - the JNI interface pointer
         * @param cls   - the Java class
         * @param field - the field ID
         *
         * @throws <code>OutOfMemoryException</code>.
         */
        FieldOnlyModifier(JNIEnv *jni, jclass cls,
                jfieldID field) throw(AgentException) :
            RequestModifier(JDWP_MODIFIER_FIELD_ONLY),
            m_field(field)
        {
            m_class = static_cast<jclass>(jni->NewGlobalRef(cls));
            if (m_class == 0) {
                throw OutOfMemoryException();
            }
        }

        /**
         * A destructor.
         */
        ~FieldOnlyModifier() {
            GetJniEnv()->DeleteGlobalRef(m_class);
        }

        /**
         * Gets the Java class.
         *
         * @return The Java class.
         */
        jclass GetClass() const throw() {
            return m_class;
        }

        /**
         * Gets the field ID.
         *
         * @return The field ID.
         */
        jfieldID GetField() const throw() {
            return m_field;
        }

        /**
         * Applies filtering by field to the given location for the given event.
         *
         * @param jni    - the JNI interface pointer
         * @param eInfo  - the request event information
         *
         * @return Returns <code>TRUE</code>, if the event field is equal to the given one.
         */
        bool Apply(JNIEnv* jni, EventInfo &eInfo) throw()
        {
            JDWP_ASSERT(eInfo.cls != 0);
            return (eInfo.field == m_field &&
                JNI_TRUE == jni->IsSameObject(eInfo.cls, m_class));
        }

    private:

        jclass m_class;
        jfieldID m_field;

    };

    /**
     * The class implements the Step modifier enabling the requested 
     * step events to be reported only if they occur within specified depth
     * and size boundaries.
     */
    class StepModifier : public RequestModifier {

    public:

        /**
         * A constructor.
         *
         * @param jni     - the JNI interface pointer
         * @param thread  - the Java thread
         * @param size    - the step size
         * @param depth   - the step depth
         *
         * @throws OutOfMemoryException.
         */
        StepModifier(JNIEnv *jni, jthread thread, jint size,
                jint depth) throw(AgentException) :
            RequestModifier(JDWP_MODIFIER_STEP),
            m_size(size),
            m_depth(depth)
        {
            m_thread = jni->NewGlobalRef(thread);
            if (m_thread == 0) {
                throw OutOfMemoryException();
            }
        }

        /**
         * A destructor.
         */
        ~StepModifier() {
            GetJniEnv()->DeleteGlobalRef(m_thread);
        }

        /**
         * Gets the Java thread.
         *
         * @return The Java thread.
         */
        jthread GetThread() const throw() {
            return m_thread;
        }

        /**
         * Gets the step size.
         *
         * @return The int step size.
         */
        jint GetSize() const throw() {
            return m_size;
        }

        /**
         * Gets the step depth.
         *
         * @return The int step depth.
         */
        jint GetDepth() const throw() {
            return m_depth;
        }

        /**
         * Applies filtering by step depth and size to the given location for 
         * the given event.
         *
         * @param jni    - the JNI interface pointer
         * @param eInfo  - the request-event information
         *
         * @return <code>TRUE</code>.
         */
        bool Apply(JNIEnv* jni, EventInfo &eInfo) throw() {
            return true;
        }

    private:

        jthread m_thread;
        jint m_size;
        jint m_depth;

    };

    /**
     * The class implements <code>InstanceOnly</code> modifier that allows 
     * the requested events to be reported only for specified object instance.
     */
    class InstanceOnlyModifier : public RequestModifier {

    public:

        /**
         * A constructor.
         *
         * @param jni - the JNI interface pointer
         * @param obj - the Java object instance
         *
         * @throws OutOfMemoryException.
         */
        InstanceOnlyModifier(JNIEnv *jni, jobject obj) throw(AgentException) :
            RequestModifier(JDWP_MODIFIER_INSTANCE_ONLY)
        {
            if (obj == 0) {
                m_instance = 0;
            } else {
                m_instance = jni->NewGlobalRef(obj);
                if (m_instance == 0) {
                    throw OutOfMemoryException();
                }
            }
        }

        /**
         * A destructor.
         */
        ~InstanceOnlyModifier() {
            GetJniEnv()->DeleteGlobalRef(m_instance);
        }

        /**
         * Gets the Java object instance.
         *
         * @return The Java object.
         */
        jobject GetInstance() const throw() {
            return m_instance;
        }

        /**
         * Applies filtering by object instance to the given location for 
         * the given event.
         *
         * @param jni   - the JNI interface pointer
         * @param eInfo - the request event information
         *
         * @return <code>TRUE</code>.
         */
        bool Apply(JNIEnv* jni, EventInfo &eInfo) throw()
        {
            if (eInfo.instance == 0 &&
                (eInfo.kind == JDWP_EVENT_SINGLE_STEP  ||
                 eInfo.kind == JDWP_EVENT_BREAKPOINT   ||
                 eInfo.kind == JDWP_EVENT_EXCEPTION    ||
                 eInfo.kind == JDWP_EVENT_METHOD_ENTRY ||
                 eInfo.kind == JDWP_EVENT_METHOD_EXIT))
            {
                jint modifiers;
                jvmtiError err;
                JVMTI_TRACE(err, GetJvmtiEnv()->GetMethodModifiers(
                    eInfo.method, &modifiers));
                if (err == JVMTI_ERROR_NONE && (modifiers & ACC_STATIC) == 0) {
                    // get "this" object from slot 0 (stated in JVM spec)
                    JVMTI_TRACE(err, GetJvmtiEnv()->GetLocalObject(
                        eInfo.thread, 0, 0, &eInfo.instance));
                }
            }
            return ((eInfo.instance == 0 && m_instance == 0) ||
                ((eInfo.instance != 0 && m_instance != 0) &&
                 JNI_TRUE == jni->IsSameObject(eInfo.instance, m_instance)));
        }

    private:

        jobject m_instance;

    };

}

#endif // _REQUEST_MODIFIER_H_
