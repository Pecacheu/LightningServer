//LightningHTTP ©2021 Pecacheu; GNU GPL 3.0
#include <server.h>
#include <unistd.h>
#include <csignal>

using namespace server; //Includes Server, HTTP, Net, and Utils namespace

//Config Options:
#define PORT 80 //HTTP Port
#define SPORT 0//443 //HTTPS Port, 0 to disable HTTPS
#define ROOT "./web" //Web Root Dir
#define CRT_FILE "web.crt" //HTTPS Cert Chain
#define KEY_FILE "web.key" //HTTPS Private Key
#define HOST "example.com" //Site Hostname

size_t RAM() { return sysconf(_SC_PHYS_PAGES)*sysconf(_SC_PAGE_SIZE); }
const size_t SystemRam=RAM(), CacheMax=SystemRam/4; //Ex. 8GB RAM -> 2GB Max Cache
WebServer *srv; //Global WebServer instance

/*A helpful example of how to use httpOpenRequest() to make a
	synchronous server-side HTTP/HTTPS request.*/
struct ReqData {
	int err=0; string msg; Buffer data; stringmap hd;
};
ReqData httpReq(NetAddr& a, const char *path, bool https=0) {
	volatile bool s=1; ReqData r;
	HttpResponse *rw=httpOpenRequest(a, [&s,&r](int err, HttpRequest *res, string *eMsg) {
		if(err || res->code != 200) {
			r.err=err?err:res->code;
			r.msg="Proxy Error: "+(eMsg?*eMsg:res?to_string(res->code)+" "+res->type:to_string(err));
		}
		if(res) { r.data=res->content.copy(); r.hd=res->header; }
		s=0;
	}, https);
	if(rw) rw->writeHead(path), rw->end();
	while(s) this_thread::sleep_for(1ms); return r;
}

//--------------------------------------------------------------------------------------------------
//------------------------------------- Request Handlers -------------------------------------------
//--------------------------------------------------------------------------------------------------

/*The full request handler order is as follows:
ServerOpt.preReq - Called before request parsing begins, generally for advanced use, ex. IP bans.
ServerOpt.onReq - Custom request handler. If you ignore a request, the server will handle it.
ServerOpt.setHdr - Called before the server sends a response, allowing custom headers to be set.
ServerOpt.readCustom - Custom file reader, ex. can be used for JS minification or PHP parsing.
ServerOpt.postReq - Called after a request is completed, useful for logging purposes.*/

/*As an example, let's say we have some files under /web/logs that we don't want cached,
	but we want to set the cache policy for everything else to 1 hour.*/
void setHdr(HttpRequest& req, HttpResponse& res, stringmap& hd) {
	if(!startsWith(req.path, "/logs")) hd["Cache-Control"] = "private, max-age=3600";
}

/*The request handler is called whenever a client makes a request to the server!
	req is your HttpRequest, containing info about the client and the request data.
	res is your HttpResponse, which allows you to reply to the client with a custom resource.
	If you've used Node.js before, this should be pretty familiar.
	Note: If you don't call res.end() or res.sendCode() before the function ends,
	the server will serve the client automatically.*/
void onReq(HttpRequest& req, HttpResponse& res) {
	string& path=req.path, host=req.header["Host"]; //Example of how to read a request header

	//HTTPS Auto-Redirect:
	if(SPORT && !req.cli.srv->sl) {
		res.setUseChunked(0); res.setKeepAlive(0);
		stringmap hd; hd["Location"] = "https://"+host+req.path;
		res.writeHead(308,&hd); res.end(); ((int&)req.u)=2; return;
	}

	//Custom Requests:
	if(path == "/ping") {
		res.setUseChunked(0);
		//Disable chunked transfer (This also means calling res.writeHead() is optional)
		res.setKeepAlive(0); //Drop the connection immediately
		res.write(httpGetVersion()+"\nPing OK"); //Write response
		//(Note: The string will be auto-converted to a Buffer object)
		res.end(); //End request
		((int&)req.u)=1; return; //Use req.u to disable logging
	} else if(req.path == "/makemeacoffee") {
		res.sendCode(418, "I'm a teapot", "Apologies, the server was unable to brew your coffee, because it is not a coffee machine."); return;
	}

	//Web Proxy Example:
	/*if(host == "wiki.example.com") {
		//Let's say there was a server at 127.0.0.1:8080 you wanted to forward traffic from.
		//How to do it? Here's a helpful example.
		//Read Proxy
		NetAddr a("127.0.0.1",8080); ReqData r=httpReq(a,path.data());
		//If err < 0, it indicates an internal error (Code 500)
		if(r.err < 0 || (r.msg.size() && !r.data.len)) { res.sendCode(500,r.msg); return; }
		//Send Response
		stringmap hd; hd["Content-Type"] = r.hd["Content-Type"];
		res.writeHead(r.err||200, r.msg, &hd); //Write header with err code, or 200 if no err
		res.write(r.data); //Write data from proxy server
		res.end(); r.data.del(); return; //Always remember to delete your Buffers when finished!
	}*/

	//File Proxy Example:
	/*if(startsWith(path,"/wiki")) {
		if(path.find("..",1) != NPOS) path="/"; //Prevent exploits
		string ex=fileExt(path), p="./wiki/"+path.substr(min((size_t)6,path.size()));
		if(p.size()<8) p+="index.html",ex="html"; //Index redirect
		cout << "Check path " << p << '\n';
		Buffer b=readFile(p); //Read file (Function from server.h)
		if(b.len == NPOS) { res.sendCode(404,"Not Found"); return; }

		//Set MIME Type Header:
		stringmap hd; auto i=ContentTypes.find(ex);
		if(i != ContentTypes.end()) hd["Content-Type"] = i->second;

		//Send Response:
		res.writeHead(200, &hd); res.write(b);
		res.end(); b.del(); return;
		//Remember to free allocated Buffers, with Buffer.del() when finished!
		//Don't worry, if you try to double-free a deleted buffer, b.del() will fail silently
	}*/
}

//Log Requests:
void logReq(HttpRequest& req, HttpResponse& res) {
	string& m=res.getStatMsg(); uint16_t s=res.getStat(); //Get response message and code
	string msg, ip=req.cli.cli.addr.host; //This could be easier to obtain, but hey
	if(s==308) msg = "\e[33mHTTPS Redirect"; //308 = Permanent Redirect
	else msg = (s>=400&&s<500?"\e[31m":"")+to_string(s)+(m.size()?' '+m:""); //Log status code
	cout << "\e[36m"+getDate(0,1)+" @ "+ip+" \e[0m"+req.uri+" -> "+msg+"\e[0m\n";
}

/*The postReq handler is similar to onReq, but called after a
	client is served. This is useful for logging purposes.*/
void postReq(HttpRequest& req, HttpResponse& res) {
	/*req.u is short for 'user', and is a custom field you can use for whatever you want.
	By default it's set to NULL, so we'll use it as a boolean to enable/disable logging.*/
	if(!req.u) logReq(req,res);
}

//--------------------------------------------------------------------------------------------------
//----------------------------------------- Main Loop ----------------------------------------------
//--------------------------------------------------------------------------------------------------

void onSig(int s) { srv->stop(); } //Signal callback calls srv->stop()
int main(int argc, char *argv[]) { //Main program
	signal(SIGTERM, onSig); signal(SIGINT, onSig); //Terminates server on SIGTERM or SIGINT (Ctrl+C)
	cout << "Date: " << getDate() << ", RAM: " << (SystemRam/1000000)
		<< "MB, Max Cache: " << (CacheMax/1000000) << "MB\n";
	/*^ Print info to console. The getDate() function is from the C-Utils library.
	NOTE: You might want to check out the other helpful functions in C-Utils that'll make writing
	in C++ a breeze. Some great ones are Buffer, fromQuery/toQuery, runCmd, and EventLoop.*/

	ServerOpt o; //Create server options list.
	//o.chkMode=0; //Example config option. This would disable Chunked Transfer Encoding
	o.setHdr=setHdr; o.onReq=onReq; o.postReq=postReq; //Set callbacks
	srv = new WebServer(ROOT,CacheMax,o); //Create WebServer

	SSLList sl(1); //Create an SSLList with size 1. Note: SSLList is only used if SPORT != 0
	if(SPORT && ckErr(sl.add(CRT_FILE, KEY_FILE, HOST),"Cert Load")) return 1;
	//^Add the certificate with sl.add(cert, key, host). Note: ckErr is a function from C-Utils
	return srv->init("Server",PORT,SPORT,&sl);
	/*^ Run WebServer. This function should be run on the main thread.
	To run a function within the server thread, make use of the EventLoop object srv->evl
	See more info on the EventLoop class @ https://github.com/pecacheu/c-utils#class-eventloop */
}