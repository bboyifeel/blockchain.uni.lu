#include <iostream>
#include <string>
#include <fstream>
#include <thread>
#include <chrono>
#include <SDL2/SDL.h>

#define LIBRARY_FILENAME "using_bulletproofs/target/release/libusing_bulletproofs.dylib"

typedef char* (*create_bullet)(uint64_t);

namespace utils {
	// remove file
	void cleanUp(std::string filename) {
		if (std::remove(filename.c_str())!=0){
			// std::cout << "Nothing to delete" << std::endl;
		}
		else {
			// std::cout << filename << " has been successfully deleted" << std::endl;
		}
	}
}// namespace utils


class Client {
private:
	uint32_t age = 0;
	std::string address;
	std::string proof;
public:
	Client(	uint32_t age, std::string address);
	std::string sendMoney(double amount, std::string recvAddress);
private:
	void generateProof();
	std::string z_sendmany(double amount, std::string recvAddress);
	std::string getTransactionId(std::string opid);
};

Client::Client(uint32_t _age, std::string _address)
: age(_age)
, address(_address) {
	std::cout << "Age " << age << std::endl;
	std::cout << "Address " << address << std::endl;
	generateProof();
}

void Client::generateProof() {
	std::cout << "Generating proof..." << std::endl;
	
	void *library_handle = SDL_LoadObject(LIBRARY_FILENAME);
	if (library_handle == NULL) {
		std::cout << "COULD NOT LOAD LIB" << std::endl;
		exit(-1);
	}
	
	create_bullet create = (create_bullet) SDL_LoadFunction(library_handle, "create_encoded_age_bulletproof");
	if (create == NULL) {
		std::cout << "COULD NOT FIND FN create_encoded_age_bulletproof" << std::endl;
		exit(-1);
	}

	proof = create(age);

	std::cout << "Proof generation is done." << std::endl;
}

// zcash-cli z_sendmany "$ZADDR" "[{\"amount\": 0.01, \"address\": \"$FRIEND_1\"}, {\"amount\": 0.01, \"address\": \"$FRIEND_2\"}]"
std::string Client::z_sendmany(double amount, std::string recvAddress){
	int memoSize = 512;
	utils::cleanUp("opid.txt");
	std::string command = "zcash-cli z_sendmany \"";
				command += address;
				command += "\" \'[";
				// 1 block
				command += "{\"amount\": ";
				command += std::to_string(amount);
				command += ", \"address\": \"";
				command += recvAddress;
				command += "\", \"memo\": \"";
				command += proof;
				command += "\"}";

				// // 2 block
				// command += ", {\"amount\": 0";
				// command += ", \"address\": \"";
				// command += recvAddress;
				// command += "\"}";
				
				// // 3nd block
				// command += ", {\"amount\": 0";
				// command += ", \"address\": \"";
				// command += recvAddress;
				// command += "\", \"memo\": \"";
				// command += proof.substr(memoSize,memoSize);
				// command += "\"}";

				command += "]\' >> opid.txt";
	// std::cout << command << std::endl;
	system(command.c_str());
	
	std::ifstream file("opid.txt");
	std::string opid;
	getline(file, opid);
	file.close();

	std::cout << "Money has been sent. OPID " << opid << std::endl;

	return opid;
}

static inline void trimNonAlphabetical(std::string& s) {
	s.erase(std::remove_if(s.begin(), s.end(), 
		[](int c) -> bool { return !std::isalnum(c); } ), s.end());
}

// zcash-cli z_getoperationstatus '["opid-e22f8794-d01d-4e23-b4d2-dbb24cf2cd28"]'
std::string Client::getTransactionId(std::string opid) {
	std::string command = "zcash-cli z_getoperationstatus \'[\"";
				command += opid;
				command += "\"]\'";
				command += " >> operationstatus.txt";
	// std::cout << command << std::endl;

	std::string status;
	do {
		std::cout << "." << std::flush;
		const int sleepTime = 5000; // ms
		std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
		utils::cleanUp("operationstatus.txt");
		system(command.c_str());
		
		// getting status
		std::ifstream file("operationstatus.txt");
		std::string word;
		bool toBreak = false;
		while(file >> word) { //take word and print
			if(toBreak) {
				status = word;
				trimNonAlphabetical(status);
				break;
			}
			if (word == "\"status\":")
				toBreak = true;
		}		
		file.close();

	} while(status == "executing");
	std::cout << std::endl;
	
	std::string txid = "";

	if (status == "success") {
		// getting txid
		std::ifstream file("operationstatus.txt");
		std::string word;
		bool toBreak = false;
		while(file >> word) { //take word and print
			if(toBreak) {
				txid = word;
				trimNonAlphabetical(txid);
				break;
			}
			if (word == "\"txid\":")
				toBreak = true;
		}		
		file.close();
	}
	else {
		std::cout << "Transaction has failed" << std::endl;
	}

	return txid;
}

std::string Client::sendMoney(double amount, std::string recvAddress) {
	std::cout << "Sending " << amount << " to " << recvAddress << std::endl;

	std::string opid = z_sendmany(amount, recvAddress);
	std::string txid = getTransactionId(opid);

	return txid;
}


int main() {
	std::string from = "ztestsapling19xjj9mdjudlr3863a0pjcf0kwhfqdpremlhcz5sy4r9sj64xmw6z89crar50tyrgf279x2l07uu";
	std::string to = "ztestsapling1mrtff36e7as3k8wxvmzuawjck3p8je0krwy9x7krh6gstgncrx70h9qw4hv7et4v2e8q5rf2n70";
	uint32_t age = 18;
	double amount = 0.01;

	Client client {age, from};	
	std::cout << client.sendMoney(amount, to) << std::endl;
	return 0;
}	