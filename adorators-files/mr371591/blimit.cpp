#include <iostream>
#include "blimit.hpp"

//unsigned int bvalue(unsigned int method, unsigned long node_id) {
//    switch (method) {
//    default: return node_id % 42;
//    case 0: return 4;
//    case 1: return 7;
//    }
//}

//unsigned int bvalue(unsigned int method, unsigned long node_id) {
//	switch (method) {
//		case 0:
//			return 1;
//		default:
//			switch (node_id) {
//				case 0: return 2;
//				case 1: return 2;
//				default: return 1;
//			}
//	}
//}

unsigned int bvalue(unsigned int method, unsigned long node_id) {
	switch (method) {
		default:
//			std::cout << "Zwracam " << (2 * node_id + method) % 10 << std::endl;
			return (2 * node_id + method) % 10 + 1;
		case 0: return 4;
		case 1: return 7;
	}
}

//unsigned int bvalue(unsigned int method, unsigned long node_id) {
////	return (2 * node_id + method) % 10;
////	return node_id; // działa :-(
//	return 1;
//}

