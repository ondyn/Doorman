// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:firebase_database/firebase_database.dart';
import 'package:firebase_auth/firebase_auth.dart';
import 'package:firebase_core/firebase_core.dart';
import './signin_page.dart';

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await Firebase
      .initializeApp(); //options: DefaultFirebaseConfig.platformOptions);
  // await FirebaseAuth.instance.useAuthEmulator('localhost', 9099);
  runApp(const AuthExampleApp());
}

/// The entry point of the application.
///
/// Returns a [MaterialApp].
class AuthExampleApp extends StatelessWidget {
  const AuthExampleApp({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Firebase Example App',
      theme: ThemeData.dark(),
      home: const Scaffold(
        body: Doorman(),
      ),
    );
  }
}

/// Provides a UI to select a authentication type page
class Doorman extends StatefulWidget {
  const Doorman({Key? key}) : super(key: key);

  @override
  State<Doorman> createState() => _DoormanState();
}

class _DoormanState extends State<Doorman> {
  bool _opening = false;
  bool _openCheck = false;
  final FirebaseAuth _auth = FirebaseAuth.instance;

  Future<void> listenToFirebase() async {
    // final event = await FirebaseDatabase.instance.ref("door").get();
    // debugPrint("get data once: ${event.value}");

    debugPrint("starting listener...");
    FirebaseDatabase.instance.ref("door/openState").onValue.listen(
      (DatabaseEvent event) {
        debugPrint("data received: ${event.snapshot.value}");
        setState(() {
          _openCheck = event.snapshot.value as bool;
        });
      },
      onError: (Object o) {
        // final error = o as FirebaseException;
        debugPrint("error: type=${o.runtimeType.toString()} error=$o");
      },
    );
  }

  @override
  void initState() {
    _auth.userChanges().listen((event) {
      debugPrint("doorVerify $event");
      if (event != null) {
        listenToFirebase();
      }
    });
    super.initState();
  }

  final DatabaseReference _doorRef = FirebaseDatabase.instance.ref("door");

  void _pushPage(BuildContext context, Widget page) {
    Navigator.of(context) /*!*/ .push(
      MaterialPageRoute<void>(builder: (_) => page),
    );
  }

  void _handlePress(bool val) {
    setState(() => _opening = val);
    debugPrint('opening: $_opening');
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
        appBar: AppBar(
          title: const Text('Doorman'),
          actions: <Widget>[
            Padding(
                padding: const EdgeInsets.only(right: 20.0),
                child: GestureDetector(
                  onTap: () => _pushPage(context, SignInPage()),
                  child: const Icon(Icons.account_circle),
                )),
          ],
        ),
        body: GestureDetector(
          onTapDown: (TapDownDetails details) {
            _handlePress(true);
            _sendMessage(true);
          },
          onTapCancel: () {
            _handlePress(false);
            _sendMessage(false);
          },
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: <Widget>[
              const SizedBox(
                height: 20,
              ),
              const Text(
                'Servant, open the door!',
                textAlign: TextAlign.center,
                style: TextStyle(fontSize: 30),
              ),
              const SizedBox(
                height: 20,
              ),
              !_openCheck
                  ? const Text('Closed',
                      textAlign: TextAlign.center,
                      style: TextStyle(color: Colors.red, fontSize: 30))
                  : const Text("Opened",
                      textAlign: TextAlign.center,
                      style: TextStyle(color: Colors.green, fontSize: 30)),
              Expanded(
                  child: FittedBox(
                      // fit: BoxFit.fitWidth,
                      fit: BoxFit.contain,
                      // fit: BoxFit.fill,
                      // no:
                      //   fit: BoxFit.cover,
                      child: Switch(onChanged: (val) {}, value: _opening)))
            ],
          ),
        ));
  }

  void _sendMessage(bool val) {
    _doorRef.update({"open": val});
  }
}
