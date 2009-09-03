/* vim: set tabstop=4 : */
#ifndef __febird_rpc_server_h__
#define __febird_rpc_server_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
# pragma warning(disable: 4819)
#endif

/**
 @file rpc �����
 */

#include <boost/preprocessor/punctuation.hpp>
#include <boost/preprocessor/repetition.hpp>
#include <boost/thread.hpp>

//#include "../thread/ConcurrentQueue.h"
//#include "../thread/thread.h"
//#include "../thread/pipeline.h"
//#include "../io/IOException.h"
//#include "../io/SocketStream.h"

#define RPC_SERVER_SIDE

#define IF_RPC_SERVER(Server, Client) Server

#include "rpc_basic.h"
#include "arg_traits.h"
#include "server_io.h"
#include "../io/IStream.h"

namespace febird { namespace rpc {

/**
 * @{
 * rpc �ӿ�������Ҫ typedef һ����ı����� self_t
 * 
 * - ʹ�ú��Ŀ��ֻ��һ�����Ƶ� FunName �Ĳ���������������ת���� tuple
 *   - ������һ���ǣ� (T1, T2, T3) �ȵ�
 *   - ����޷����ֱ��ת��������������ʽ��ʹ�� tuple���Ϳ����� mem_fun_binder ��ת����������
 *
 * - ��һ����Ա������ΪԶ�̷���������ͨ��������������ɣ�һ��������������һ��������ע��
 *   - ����ʱ�� RPC_DECLARE_MF
 *   - ע��ʱ�� BEGIN_RPC_ADD_MF, RPC_ADD_MF, END_RPC_ADD_MF
 */

#define RPC_DECLARE_INTERFACE(Interface, Scope) \
	class Interface : public Scope { \
	public: \
	BEGIN_RPC_ADD_MF(Interface, Scope)

//! ��Զ�̶���������һ����Ա����
#define RPC_DECLARE_MF_EX(FunName, ArgList, PureVirtualTag) \
	virtual rpc_ret_t FunName ArgList PureVirtualTag \
	typedef rpc_ret_t (self_t::*FunName##_stub_t)ArgList;

#define RPC_DECLARE_MF(FunName, ArgList) RPC_DECLARE_MF_EX(FunName, ArgList, =0;)

//! ��ʼ��Զ�̶�����ע���Ա�����飬ֻע��һ��
//! ʹ������꣬�û������Լ����壺 static remote_object* create_object()
//! get_meta �ķ���ֵ pair.first ֻ��Ϊ�˴���������Ϣ
#define BEGIN_RPC_ADD_MF_EX(ThisClass,ClassName)\
	typedef ThisClass self_t;				\
public:										\
	FEBIRD_RPC_GEN_full_mf_name(ClassName)	\
	template<class Session>					\
	static std::pair<ThisClass*,ClassMeta*> \
	get_meta(Session*tag)					\
	{										\
		std::pair<ThisClass*,ClassMeta*> meta(0,0);\
		meta.second = new ClassMeta; \
		meta.second->className = ClassName; \
		meta.second->create = 0;
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define BEGIN_RPC_ADD_MF(ThisClass)	BEGIN_RPC_ADD_MF_EX(ThisClass, #ThisClass)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//! ��Զ�̶�����ע��һ����Ա����
#define RPC_ADD_MF(FunName) \
meta.second->stubs.push_back(new server_stub<Session,FunName##_stub_t>(&self_t::FunName,full_mf_name(#FunName)));

//! ������Զ�̶�����ע���Ա������
#define END_RPC_ADD_MF() return meta; }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/*
#define BEGIN_RPC_IMP_INTERFACE_EX(Impl, Interface)  \
  class Impl : public Interface {			\
  public:									\
  template<class Session>					\
  static void reg_interface					\
  (Session* sess, create_fun_t pf_create)	\
  {	ClassMeta* meta = Interface::get_meta(sess).second; \
	meta->create = pf_create; \
  }
*/

#define BEGIN_RPC_IMP_INTERFACE_EX(Impl, Interface)  \
  class Impl : public Interface { \
  public:

#define BEGIN_RPC_IMP_INTERFACE(Impl, Interface)  \
	BEGIN_RPC_IMP_INTERFACE_EX(Impl, Interface) \
	static remote_object*create_object() {return new Impl;}

#define END_RPC_IMP_INTERFACE() };

#define RPC_SERVER_AUTO_CREATE(server, ImpClass) \
	(server).auto_create((ImpClass*)0, &ImpClass::create_object)

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class server_packet_base;
class session_base;

class FEBIRD_DLL_EXPORT server_stub_i
{
public:
	std::string m_name;
	unsigned m_id;

public:
	virtual ~server_stub_i();

	virtual void invoke(server_packet_base* packet) const = 0;

	virtual void sync_call(session_base* session, const server_packet_base* packet) const = 0;
	virtual void send_args(session_base* session, server_packet_base* packet) const = 0;
	virtual server_packet_base* read_args(session_base* server, const server_packet_base* packet) const = 0;
};

struct FEBIRD_DLL_EXPORT ClassMeta : public RoCreator
{
	std::vector<server_stub_i*> stubs;
};

class FEBIRD_DLL_EXPORT server_packet_base : public RefCounter
{
public:
	server_stub_i* stub;
	unsigned     seqid;
	short		 how_call;
	bool		 isbyid;
	rpc_ret_t retv;

	void copy(const server_packet_base* header)
	{
		stub = header->stub;
		seqid = header->seqid;
		how_call = header->how_call;
		isbyid = header->isbyid;
	}

	void invoke() { stub->invoke(this); }

	template<class Input>
	void read_header(Input& input, const AccessByNameID<server_stub_i*>& stubs)
	{
		var_uint32_t vint;
		input >> vint;
		this->seqid = vint.t;
		stub = 0;
		var_uint32_t callid;
		input >> callid;
		if (0 == callid.t)
		{
			std::string funName;
			input >> funName;
			stub = stubs.get_byname(funName);
			if (0 == stub)
			{
				std::ostringstream oss;
				oss << "can not find function [name=\"" << funName << "\" to call";
				throw rpc_exception(oss.str());
			}
			isbyid = false;
		} else {
			if (!stubs.is_valid(callid.t))
			{
				std::ostringstream oss;
				oss << "can not find function id=" << callid.t << " to call";
				throw rpc_exception(oss.str());
			}
			isbyid = true;
			stub = stubs.get_byid(callid.t);
		}
		input >> vint;
		how_call = vint.t;
		assert(stub);
	}
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// declare the template prototype
template<class Function> struct server_packet;

// partial specializations, use BOOST_PP..
//
#include <boost/preprocessor/iterate.hpp>
#define BOOST_PP_ITERATION_LIMITS (0, 9)
#define BOOST_PP_FILENAME_1       <febird/rpc/pp_server_stub.h>
#include BOOST_PP_ITERATE()


template<class Session, class Function>
class server_stub :	public server_stub_i
{
	typedef arglist_val<Function> aval_t;
	typedef arglist_ref<Function> aref_t;
	typedef server_packet<Function> packet_t;

	Function m_pfun;

public:
	server_stub(Function pfun, const std::string& name);

	virtual void sync_call(session_base* vpsession, const server_packet_base* header) const;

	virtual void invoke(server_packet_base* packet) const;

	virtual server_packet_base* read_args(session_base* vpsession, const server_packet_base* header) const;
	virtual void send_args(session_base* vpsession, server_packet_base* packet) const;

private:
	void do_send_args(Session* session, packet_t* p) const;
};

//////////////////////////////////////////////////////////////////////////
class FEBIRD_DLL_EXPORT session_base : public SessionScope
{
protected:
	class rpc_server_base* m_owner;

	AccessByNameID<server_stub_i*>*m_stubTable;
	AccessByNameID<GlobaleScopePtr>* m_globaleScopeObjects;
//	AccessByNameID<GlobaleScopePtr>  m_globaleScopeObjectsRef;
	AccessByNameID<SessionScopePtr>  m_sessionScopeObjects;
	ObjectFactory<SessionScope>*   m_sessionScopeFactory;
	std::auto_ptr<boost::thread>   m_thread;
	volatile bool m_bRun;

	void call(server_packet_base& header);

	typedef session_base my_self_t;

	static remote_object* create_object();

#include "rpc_interface.h"

	virtual void read_header(server_packet_base& header) = 0;
	void run();

public:
	session_base(class rpc_server_base* owner);
	virtual ~session_base();

	void start();
};

class FEBIRD_DLL_EXPORT rpc_server_base
{
	friend class session_base;
protected:
	std::set<boost::intrusive_ptr<session_base> > m_sessionList;

	AccessByNameID<server_stub_i*> m_stubTable;
	AccessByNameID<GlobaleScopePtr> m_globaleScopeObjects;
	ObjectFactory<GlobaleScope> m_globaleScopeFactory;
	ObjectFactory<SessionScope> m_sessionScopeFactory;

	boost::mutex m_mutex_sessionList;
	boost::mutex m_mutex_stubTable;
	boost::mutex m_mutex_globaleScopeObjects;
	boost::mutex m_mutex_globaleScopeFactory;
	boost::mutex m_mutex_sessionScopeFactory;

	IAcceptor*   m_acceptor;

	void gen_stubs(ClassMeta* meta);
	bool addto_factroy(ClassMeta* meta, GlobaleScope* scopeTag);
	bool addto_factroy(ClassMeta* meta, SessionScope* scopeTag);

	void add_servant_imp(GlobaleScope* servant, const std::string& name);

public:
	rpc_ret_t createGlobaleObject(var_uint32_t* servantID, const std::string& className);
	rpc_ret_t createNamedGlobaleObject(var_uint32_t* servantID, std::string& className, const std::string& name);
	rpc_ret_t retrieveGlobaleObject(var_uint32_t* servantID, const std::string& name);

 	void remove_servant(GlobaleScopePtr servant);

	void removeSession(boost::intrusive_ptr<session_base> session);
	virtual session_base* createSession() = 0;

	rpc_server_base(IAcceptor* acceptor = 0);
	virtual ~rpc_server_base();

	void start();
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//@}

/**
 * rpc_server
 *
 * concept of Connection
 *  -# bool listen(const char* szName);
 *	-# IDuplexStream* accept(); // return a stream connected with a server side
 */
template<template<class IStream_> class Input,
		 template<class OStream_> class Output
		>
class rpc_server : public rpc_server_base
{
	typedef rpc_server self_t;
	typedef Input<InputBuffer>	  input_t;
	typedef Output<OutputBuffer>  output_t;
	typedef server_object_input<input_t>   so_input_t;
	typedef server_object_output<output_t> so_output_t;
/*
	class RpcTask : public febird::thread::PipelineTask
	{
		so_input_t* input;
		so_output_t* output;
		server_stub_i* stub;
		void*   args;
	public:
		RpcTask(so_input_t* input, so_output_t* output)
			: input(input), output(output)
		{
		}

		void income()
		{

		}
	};
*/
	class RpcSession : public session_base
	{
		std::auto_ptr<IDuplexStream> m_stream;
		 input_t   input;
		output_t  output;

	protected:
		void read_header(server_packet_base& header)
		{
			header.read_header(input, *m_stubTable);
		}
	public:
		so_input_t  m_so_input;
		so_output_t m_so_output;

		RpcSession(rpc_server* owner, IDuplexStream* stream)
			: session_base(owner)
			, m_stream(stream)
			, m_so_input(&owner->m_globaleScopeObjects, &m_sessionScopeObjects, &input)
			, m_so_output(&owner->m_globaleScopeObjects, &m_sessionScopeObjects, &output)
		{
			input.attach(stream);
			input.set_bufsize(8*1024);
			output.attach(stream);
			output.set_bufsize(8*1024);
		}
	};
	friend class RpcSession;

	session_base* createSession()
	{
		if (IDuplexStream* stream = m_acceptor->accept())
			return new RpcSession(this, stream);
		else
			return 0;
	}

public:
	rpc_server(IAcceptor* acceptor)
		: rpc_server_base(acceptor)
	{
		create_fun_t pf_create = 0;
		auto_create((RpcSession*)0, pf_create);
	}

	template<class Class> 
	void auto_create(Class* tag, create_fun_t pf_create)
	{
		ClassMeta* meta = Class::get_meta((RpcSession*)0).second;
		if (addto_factroy(meta, tag))
		{
			meta->create = pf_create;
			gen_stubs(meta);
		}
	}
	template<class Class>
	void add_servant(Class* x, const std::string& name, create_fun_t pf_create)
	{
		std::string className = Class::s_getClassName();
		if (0 == m_globaleScopeFactory.FindMetaByName(className))
		{
			ClassMeta* meta = Class::get_meta((RpcSession*)0).second;
			meta->create = pf_create;
			gen_stubs(meta);
			this->addto_factroy(meta, (Class*)0);
		}
		rpc_server_base::add_servant_imp(x, name);
	}
	template<class Class>
	void add_servant(Class* x, const std::string& name)
	{
		add_servant(x, name, &Class::create_object);
	}
	template<class Class>
	void add_servant(boost::intrusive_ptr<Class> x, const std::string& name, create_fun_t pf_create)
	{
		add_servant(x.get(), name, pf_create);
	}
	template<class Class>
	void add_servant(boost::intrusive_ptr<Class> x, const std::string& name)
	{
		add_servant(x.get(), name, &Class::create_object);
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
// template<class Session, class Function> class server_stub implementation

template<class Session, class Function>
server_stub<Session, Function>::server_stub(Function pfun, const std::string& name)
{
	this->m_name = name;
	this->m_id = 0;
	m_pfun = pfun;
}

template<class Session, class Function>
void 
server_stub<Session, Function>::sync_call(session_base* vpsession, const server_packet_base* header) const
{
	Session* session = static_cast<Session*>(vpsession);
	packet_t packet;
	packet.copy(header);
	
	aref_t refs(boost::mpl::true_(), packet.argvals);
	refs.load(session->m_so_input);
	packet.argvals.sync(refs);

	packet.invoke_f(m_pfun);
	do_send_args(session, &packet);
}

template<class Session, class Function>
void 
server_stub<Session, Function>::invoke(server_packet_base* packet) const
{
	packet_t* p = (packet_t*)packet;
	p->invoke_f(m_pfun);
}

template<class Session, class Function>
server_packet_base* 
server_stub<Session, Function>::read_args(session_base* vpsession, const server_packet_base* header) const
{
	Session* session = static_cast<Session*>(vpsession);
	packet_t* p = 0;
	try {
		p = new packet_t;
		p->copy(header);

		aref_t refs(boost::mpl::true_(), p->argvals);
		refs.load(session->m_so_input);
		p->argvals.sync(refs);
	}
	catch (const std::exception& exp)
	{
		delete p;
		p = 0;
		fprintf(stderr, "fun=%s, exp.what=%s\n", BOOST_CURRENT_FUNCTION, exp.what());
	}
	return p;
}

template<class Session, class Function>
void 
server_stub<Session, Function>::send_args(session_base* vpsession, server_packet_base* packet) const
{
	Session* session = static_cast<Session*>(vpsession);
	packet_t* p = (packet_t*)packet;
	do_send_args(session, p);
}

template<class Session, class Function>
void 
server_stub<Session, Function>::do_send_args(Session* session, packet_t* p) const
{
	session->m_so_output.get()
		<< var_uint32_t(p->seqid)
		<< p->retv;

	aref_t refs(boost::mpl::true_(), p->argvals);
	refs.save(session->m_so_output);

	if (p->isbyid)
		session->m_so_output.get() << var_uint32_t(this->m_id);
	session->m_so_output.flush();
}

////////////////////////////////////////////////////////////////////////////////////////////////

} } // namespace::febird::rpc


#endif // __febird_rpc_server_h__