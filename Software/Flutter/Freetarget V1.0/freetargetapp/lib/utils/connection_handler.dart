import 'dart:io';
import 'dart:async';
import 'dart:convert';
import '../models/shot.dart';

class ConnectionHandler {
  Socket? socket;
  bool isConnected = false;
  DateTime? lastKeepAlive;
  String incomingJSON = "";
  final void Function(String) onMessage;
  final void Function(Shot) onShot;
  final void Function() onDisconnect;

  ConnectionHandler({
    required this.onMessage,
    required this.onShot,
    required this.onDisconnect,
  });

  Future<void> connect(String ip, int port, TargetType targetType) async {
    if (isConnected) {
      await disconnect();
      return;
    }

    try {
      onMessage('Connecting to ETS...');
      socket = await Socket.connect(ip, port);
      isConnected = true;
      lastKeepAlive = DateTime.now();
      onMessage('Connected to ETS');

      socket!.listen(
        (data) => _handleIncomingData(data, targetType),
        onError: _handleError,
        onDone: _handleDisconnection,
      );
    } catch (e) {
      isConnected = false;
      onMessage('Connection failed: ${e.toString()}');
    }
  }

  void _handleIncomingData(List<int> data, TargetType targetType) {
    String message = utf8.decode(data);
    
    onMessage('Original message: $message');

    lastKeepAlive = DateTime.now();
    incomingJSON += message;
    incomingJSON = incomingJSON.replaceAll(", ,", ",,").replaceAll(",,", ",");

    int indexOpenBracket = 0;
    int indexClosedBracket = 0;

    try {
      indexOpenBracket = incomingJSON.indexOf('{');
      if (indexOpenBracket > -1) {
        indexClosedBracket = incomingJSON.indexOf('}', indexOpenBracket);
        if (indexClosedBracket > -1) {
          message = incomingJSON.substring(
              indexOpenBracket, indexClosedBracket + 1);
          incomingJSON = incomingJSON.substring(indexClosedBracket + 1);
          onMessage('json message: $message');

          if (message.contains('KEEP_ALIVE')) {
            onMessage('Keep alive received');
          } else if (message.contains('shot')) {
            _processShotData(message, targetType);
          }

          onMessage('Remaining JSON buffer: $incomingJSON');
        }
      }
    } catch (e) {
      onMessage('Error in JSON buffer: $incomingJSON');
      onMessage('JSON parsing indexes - Start: $indexOpenBracket, End: $indexClosedBracket');
      onMessage('Error processing data: $e');
      incomingJSON = "";
    }
  }

  void _processShotData(String message, TargetType targetType) {
    try {
      final json = jsonDecode(message);
      final shot = Shot.fromJson(json, targetType);
      onShot(shot);
    } catch (e) {
      onMessage('Error processing shot: $e');
    }
  }

  void _handleError(error) {
    isConnected = false;
    onMessage('Connection error: ${error.toString()}');
    onDisconnect();
  }

  void _handleDisconnection() {
    isConnected = false;
    onMessage('Disconnected from ETS');
    onDisconnect();
  }

  Future<void> disconnect() async {
    socket?.destroy();
    isConnected = false;
    onMessage('Disconnected from ETS');
  }

  void sendSettings(Map<String, dynamic> settings) {
    if (isConnected) {
      final settingsJson = jsonEncode(settings);
      socket?.write('{$settingsJson}');
      onMessage('Settings applied: $settingsJson');
    }
  }

  void dispose() {
    socket?.destroy();
  }
}