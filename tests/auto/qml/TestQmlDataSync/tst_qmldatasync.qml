import QtQuick 2.5
import de.skycoder42.QtDataSync 4.0
import QtTest 1.1

Item {
	id: root

	TestCase {
		name: "DataStore"

		DataStore {
			id: store
		}

		DataStoreModel {
			id: storeModel1
		}

		DataStoreModel {
			id: storeModel2
			dataStore: store
		}

		SyncManager {
			id: syncManager
		}

		AccountManager {
			id: accountManager
		}

		UserExchangeManager {
			id: exchangeManager1
		}

		UserExchangeManager {
			id: exchangeManager2
			manager: accountManager
		}

		function test_valid() {
			verify(store.valid);
			verify(storeModel1.valid);
			verify(storeModel2.valid);
			verify(syncManager.valid);
			verify(accountManager.valid);
			verify(exchangeManager1.valid);
			verify(exchangeManager2.valid);
		}
	}

}
