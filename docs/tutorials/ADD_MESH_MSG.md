# Add a new mesh message

This guide shows how to add a new mesh message, how to send it, how to handle a received message, and how to send a reply.
We use an example where the sender sets fields `a` and `b` to a value, while the receiver adds up the two values and sends that in the reply.

Start by adding a new type to `cs_mesh_model_msg_type_t`. For example:
```
CS_MESH_MODEL_TYPE_EXAMPLE = 123
```

Then define a struct with the contents of the message in `cs_MeshModelPackets.h`. For example:
```
struct __attribute__((__packed__)) cs_mesh_model_msg_example_t {
	uint8_t  a;
	uint8_t  b;
	uint16_t sum;
};
```

Note that the type and struct are protocol, and thus are hard to change after a release.

Add the new type to `isValidMeshPayload()`. For example:
```
	case CS_MESH_MODEL_TYPE_EXAMPLE:
		return payloadSize == sizeof(cs_mesh_model_msg_example_t);
```

Add the new type with the new struct to `cs_MeshMsgEvent.h`. For example:
```
template<>
struct MeshPacketTraits<CS_MESH_MODEL_TYPE_EXAMPLE> {
	using type = cs_mesh_model_msg_example_t;
};
```

Now, let's send an example mesh message:
```
cs_ret_code_t Example::sendMsg(stone_id_t stoneId) {
	cs_mesh_model_msg_example_t payload;
	payload.a = 13;
	payload.b = 29;

	cs_mesh_msg_t meshMsg;
	meshMsg.type                    = CS_MESH_MODEL_TYPE_EXAMPLE;
	meshMsg.flags.flags.broadcast   = false;
	meshMsg.flags.flags.acked       = true;
	meshMsg.flags.flags.useKnownIds = false;
	meshMsg.flags.flags.doNotRelay  = false;
	meshMsg.reliability             = 0;
	meshMsg.urgency                 = CS_MESH_URGENCY_LOW;
	meshMsg.idCount                 = 1;
	meshMsg.targetIds               = &stoneId;
	meshMsg.payload                 = reinterpret_cast<uint8_t*>(&payload);
	meshMsg.size                    = sizeof(payload);

	event_t event(CS_TYPE::CMD_SEND_MESH_MSG, &meshMsg, sizeof(meshMsg));
	event.dispatch();
	return event.result.returnCode;
}
```
This is a lot of code, but most are simply options for how to send the mesh message.
In the example, we send a reliable message, so that a reply should be sent back.


Next, we want to handle incoming messages.
Right now, a lot of mesh messages are handled in the `MeshMsgHandler` class. However, we are moving away from that. You can simply handle the mesh message in your event handler. For example:
```
void Example::handleEvent(event_t &event) {
	switch (event.type) {
		case CS_TYPE::EVT_RECV_MESH_MSG: {
			auto packet = CS_TYPE_CAST(EVT_RECV_MESH_MSG, event.data);
			onMeshMsg(*packet);
			break;
		}
		default: {
			break;
		}
	}
}

void Example::onMeshMsg(MeshMsgEvent& msg) {
	if (msg.type != CS_MESH_MODEL_TYPE_EXAMPLE) {
		return;
	}
	auto payload = msg.getPacket<CS_MESH_MODEL_TYPE_EXAMPLE>();

	if (msg.isReply) {
		LOGi("Received reply: sum=%u", payload.sum);
		return;
	}

	LOGi("Received msg: a=%u b=%u", payload.a, payload.b);

	// If the reply is not null, set the mesh msg type and data to reply with.
	// We reply with the same message, with the same data format
	// (so that it passes the `isValidMeshPayload()` check).
	// But you can reply with anything you want.
	if (msg.reply != nullptr && msg.reply->buf.len >= sizeof(cs_mesh_model_msg_example_t)) {
		msg.reply->type = CS_MESH_MODEL_TYPE_EXAMPLE;

		auto replyPayload = reinterpret_cast<cs_mesh_model_msg_example_t*>(msg.reply->data);
		replyPayload->sum = payload.a + payload.b;
		msg.reply->dataSize = sizeof(cs_mesh_model_msg_example_t);
	}
}

```
As mentioned in the comments, you can send any mesh message as reply, by simply filling the reply type and data.

Note that acked mesh messages that are sent as a result of a [mesh command](../protocol/PROTOCOL.md#mesh-command-packet) via UART, should send the result(s) of the control command via UART.
