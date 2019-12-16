import * as fbfn from "firebase-functions";
import * as fbadm from "firebase-admin";
import { FieldValue } from "@google-cloud/firestore";

fbadm.initializeApp();

const documentId = "/datasync/{userId}/{dataType}/{dataId}";

const createTrigger = fbfn
    .region("europe-west1")
    .firestore
    .document(documentId)
    .onCreate(async (change, context) => {
        let data = change.data();
        if (data) {
            await change.ref.update({
                timestamp: FieldValue.serverTimestamp(),
                svrModified: true
            });
        }
    });

const updateTrigger = fbfn
    .region("europe-west1")
    .firestore
    .document(documentId)
    .onUpdate(async (change, context) => {
        const oldData = change.before.data();
        let newData = change.after.data();
        if (newData && oldData) {
            if (newData.svrModified)  // skip entries marked as "server modified"
                return;

            if (newData.version <= oldData.version)
                await change.after.ref.set(oldData);
            else if (newData.isDeletion)
                await change.after.ref.delete();
            else {
                await change.after.ref.update({
                    timestamp: FieldValue.serverTimestamp(),
                    svrModified: true
                });
            }
        }
    });

export { createTrigger, updateTrigger };