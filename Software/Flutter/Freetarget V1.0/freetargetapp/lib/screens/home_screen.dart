import 'package:flutter/material.dart';
import 'dart:async';
import '../models/series.dart';
import '../models/shot.dart';
import '../models/settings.dart';
import '../painters/target_painters.dart';
import '../utils/connection_handler.dart';
import 'session_review_screen.dart';
import '../painters/group_analysis_painter.dart';

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  // Socket and connection
  late ConnectionHandler connectionHandler;
  bool isConnected = false;
  String lastMessage = 'No messages';
  List<String> recentMessages = [];

  // Target and scoring
  List<Series> allSeries = [];
  int currentSeriesIndex = 0;
  double totalScore = 0;
  double totalDecimalScore = 0;
  double zoomLevel = 2.0;
  bool useDecimalScoring = true;
  bool isStatusExpanded = false;
  TargetType targetType = TargetType.airPistol;
  bool showAnalysis = false;

  // Target swipe control
  final PageController _targetPageController = PageController();
  bool _canSwipeTargets = false;

  // Settings
  ETSSettings settings = ETSSettings();

  // Timer state
  bool isTimerEnabled = false;
  Timer? _timer;
  Duration _currentDuration = const Duration(minutes: 75);
  Duration _selectedDuration = const Duration(minutes: 75);
  bool isTimerPaused = false;

  // Controllers
  final TextEditingController ipController =
      TextEditingController(text: '192.168.10.9');
  final TextEditingController portController =
      TextEditingController(text: '1090');

  @override
  void initState() {
    super.initState();
    allSeries = List.generate(6, (index) => Series());
    _initializeConnectionHandler();
  }

  void _initializeConnectionHandler() {
    connectionHandler = ConnectionHandler(
      onMessage: _addMessage,
      onShot: _handleShot,
      onDisconnect: () {
        setState(() {
          isConnected = false;
        });
      },
    );
  }

  void _addMessage(String message) {
    setState(() {
      recentMessages.insert(
          0, '${DateTime.now().toString().split('.')[0]} - $message');
      if (recentMessages.length > 5) {
        recentMessages.removeLast();
      }
      lastMessage = message;
    });
  }

  Future<void> _connect() async {
    if (isConnected) {
      await connectionHandler.disconnect();
      setState(() {
        isConnected = false;
      });
      return;
    }

    try {
      await connectionHandler.connect(
        ipController.text,
        int.parse(portController.text),
        targetType,
      );
      setState(() {
        isConnected = true;
      });
    } catch (e) {
      setState(() {
        isConnected = false;
      });
    }
  }

  void _handleShot(Shot shot) {
    setState(() {
      Series currentSeries = allSeries[currentSeriesIndex];
      currentSeries.shots.add(shot);
      currentSeries.totalScore += shot.score;
      currentSeries.decimalTotalScore += shot.decimalScore;
      totalScore += shot.score;
      totalDecimalScore += shot.decimalScore;

      _addMessage(
          'Shot scored: ${shot.decimalScore.toStringAsFixed(1)}${shot.isInnerTen ? '*' : ''}');

      if (currentSeries.shots.length == 10) {
        _handleSeriesComplete();
      }
    });
  }

  void _handleSeriesComplete() {
    if (currentSeriesIndex < 5) {
      setState(() {
        currentSeriesIndex++;
        _canSwipeTargets = true;
        _targetPageController.animateToPage(
          currentSeriesIndex,
          duration: const Duration(milliseconds: 300),
          curve: Curves.easeInOut,
        );
      });
    } else {
      _showSessionReview();
    }
  }

  void _showSessionReview() {
    Navigator.push(
      context,
      MaterialPageRoute(
        builder: (context) => SessionReviewScreen(
          allSeries: allSeries,
          targetType: targetType,
          useDecimalScoring: useDecimalScoring,
          totalScore: totalScore,
          totalDecimalScore: totalDecimalScore,
          sessionStartTime: DateTime.now(),
        ),
      ),
    );
  }

  // Timer functions
  void _startTimer() {
    _timer?.cancel();
    _timer = Timer.periodic(const Duration(seconds: 1), (timer) {
      if (!isConnected || isTimerPaused) return;

      setState(() {
        if (_currentDuration.inSeconds > 0) {
          _currentDuration = Duration(seconds: _currentDuration.inSeconds - 1);
        } else {
          _timer?.cancel();
          _showTimeCompleteDialog();
        }
      });
    });
  }

  void _resetTimer() {
    setState(() {
      _currentDuration = _selectedDuration;
      isTimerPaused = false;
    });
    _startTimer();
  }

  void _toggleTimer() {
    setState(() {
      isTimerPaused = !isTimerPaused;
    });
  }

  void _showDurationPicker() {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Select Timer Duration'),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            ListTile(
              title: const Text('15 minutes'),
              onTap: () {
                _updateDuration(const Duration(minutes: 15));
                Navigator.pop(context);
              },
            ),
            ListTile(
              title: const Text('1 hour 15 minutes'),
              onTap: () {
                _updateDuration(const Duration(minutes: 75));
                Navigator.pop(context);
              },
            ),
            ListTile(
              title: const Text('Custom'),
              onTap: () {
                Navigator.pop(context);
                _showCustomDurationPicker();
              },
            ),
          ],
        ),
      ),
    );
  }

  void _showCustomDurationPicker() {
    int hours = 0;
    int minutes = 0;

    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Custom Duration'),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Row(
              children: [
                Expanded(
                  child: TextField(
                    decoration: const InputDecoration(
                      labelText: 'Hours',
                      border: OutlineInputBorder(),
                    ),
                    keyboardType: TextInputType.number,
                    onChanged: (value) => hours = int.tryParse(value) ?? 0,
                  ),
                ),
                const SizedBox(width: 16),
                Expanded(
                  child: TextField(
                    decoration: const InputDecoration(
                      labelText: 'Minutes',
                      border: OutlineInputBorder(),
                    ),
                    keyboardType: TextInputType.number,
                    onChanged: (value) => minutes = int.tryParse(value) ?? 0,
                  ),
                ),
              ],
            ),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
          TextButton(
            onPressed: () {
              _updateDuration(
                Duration(hours: hours, minutes: minutes),
              );
              Navigator.pop(context);
            },
            child: const Text('Set'),
          ),
        ],
      ),
    );
  }

  void _updateDuration(Duration duration) {
    setState(() {
      _selectedDuration = duration;
      _currentDuration = duration;
      isTimerPaused = false;
    });
    _startTimer();
  }

  void _showTimeCompleteDialog() {
    showDialog(
      context: context,
      barrierDismissible: false,
      builder: (context) => AlertDialog(
        title: const Text('Time Complete'),
        content: const Text('Would you like to review this session?'),
        actions: [
          TextButton(
            onPressed: () {
              Navigator.pop(context);
              _showSessionReview();
            },
            child: const Text('Review Session'),
          ),
          TextButton(
            onPressed: () {
              setState(() {
                for (var series in allSeries) {
                  series.reset();
                }
                currentSeriesIndex = 0;
                totalScore = 0;
                totalDecimalScore = 0;
              });
              Navigator.pop(context);
            },
            child: const Text('Reset Without Review'),
          ),
        ],
      ),
    );
  }

  void _resetScores() {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Reset Scores'),
        content: const Text(
            'Would you like to review this session before resetting?'),
        actions: [
          TextButton(
            onPressed: () {
              Navigator.pop(context);
              _showSessionReview();
            },
            child: const Text('Review Session'),
          ),
          TextButton(
            onPressed: () {
              setState(() {
                for (var series in allSeries) {
                  series.reset();
                }
                currentSeriesIndex = 0;
                totalScore = 0;
                totalDecimalScore = 0;
                _addMessage('Scores reset');
              });
              Navigator.pop(context);
            },
            child: const Text('Reset Without Review'),
          ),
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
        ],
      ),
    );
  }

  void _showSettingsModal() {
    showModalBottomSheet(
      context: context,
      isScrollControlled: true,
      backgroundColor: Colors.transparent,
      builder: (context) => StatefulBuilder(
        builder: (context, setModalState) => Container(
          height: MediaQuery.of(context).size.height * 0.8,
          decoration: const BoxDecoration(
            color: Colors.white,
            borderRadius: BorderRadius.vertical(top: Radius.circular(20)),
          ),
          child: Column(
            children: [
              Padding(
                padding: const EdgeInsets.all(16),
                child: Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    const Text(
                      'Hardware Settings',
                      style: TextStyle(
                        fontSize: 20,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                    IconButton(
                      icon: const Icon(Icons.close),
                      onPressed: () => Navigator.pop(context),
                    ),
                  ],
                ),
              ),
              const Divider(),
              Expanded(
                child: SingleChildScrollView(
                  padding: const EdgeInsets.all(16),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      const Text(
                        'LED Brightness',
                        style: TextStyle(
                          fontSize: 16,
                          fontWeight: FontWeight.bold,
                        ),
                      ),
                      Row(
                        children: [
                          Expanded(
                            child: Slider(
                              value: settings.ledBrightness.toDouble(),
                              min: 0,
                              max: 99,
                              divisions: 99,
                              onChanged: (value) {
                                setModalState(() {
                                  settings.ledBrightness = value.round();
                                });
                              },
                            ),
                          ),
                          Text('${settings.ledBrightness}%'),
                        ],
                      ),
                      const SizedBox(height: 24),
                      const Text(
                        'Paper Feed Mode',
                        style: TextStyle(
                          fontSize: 16,
                          fontWeight: FontWeight.bold,
                        ),
                      ),
                      const SizedBox(height: 8),
                      RadioListTile<bool>(
                        title: const Text('DC Motor'),
                        value: true,
                        groupValue: settings.isDCMotor,
                        onChanged: (value) {
                          setModalState(() {
                            settings.isDCMotor = value!;
                          });
                        },
                      ),
                      if (settings.isDCMotor)
                        Padding(
                          padding: const EdgeInsets.symmetric(horizontal: 16),
                          child: TextField(
                            decoration: const InputDecoration(
                              labelText: 'Time (ms)',
                              border: OutlineInputBorder(),
                            ),
                            keyboardType: TextInputType.number,
                            controller: TextEditingController(
                              text: settings.paperTime.toString(),
                            ),
                            onChanged: (value) {
                              settings.paperTime = int.tryParse(value) ?? 1000;
                            },
                          ),
                        ),
                      RadioListTile<bool>(
                        title: const Text('Stepper Motor'),
                        value: false,
                        groupValue: settings.isDCMotor,
                        onChanged: (value) {
                          setModalState(() {
                            settings.isDCMotor = value!;
                          });
                        },
                      ),
                      if (!settings.isDCMotor)
                        Padding(
                          padding: const EdgeInsets.symmetric(horizontal: 16),
                          child: Column(
                            children: [
                              TextField(
                                decoration: const InputDecoration(
                                  labelText: 'Step Count',
                                  border: OutlineInputBorder(),
                                ),
                                keyboardType: TextInputType.number,
                                controller: TextEditingController(
                                  text: settings.stepperCount.toString(),
                                ),
                                onChanged: (value) {
                                  settings.stepperCount =
                                      int.tryParse(value) ?? 100;
                                },
                              ),
                              const SizedBox(height: 8),
                              TextField(
                                decoration: const InputDecoration(
                                  labelText: 'Step Time (ms)',
                                  border: OutlineInputBorder(),
                                ),
                                keyboardType: TextInputType.number,
                                controller: TextEditingController(
                                  text: settings.stepperTime.toString(),
                                ),
                                onChanged: (value) {
                                  settings.stepperTime =
                                      int.tryParse(value) ?? 1000;
                                },
                              ),
                            ],
                          ),
                        ),
                    ],
                  ),
                ),
              ),
              Padding(
                padding: const EdgeInsets.all(16),
                child: ElevatedButton(
                  onPressed: () {
                    _applySettings();
                    Navigator.pop(context);
                  },
                  style: ElevatedButton.styleFrom(
                    minimumSize: const Size.fromHeight(50),
                  ),
                  child: const Text('Apply Settings'),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  void _applySettings() {
    if (isConnected) {
      connectionHandler.sendSettings(settings.toJson());
    } else {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('Please connect to ETS first'),
          duration: Duration(seconds: 2),
        ),
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    String timeString = '';
    if (_currentDuration.inHours > 0) {
      timeString =
          '${_currentDuration.inHours}:${(_currentDuration.inMinutes % 60).toString().padLeft(2, '0')}:${(_currentDuration.inSeconds % 60).toString().padLeft(2, '0')}';
    } else {
      timeString =
          '${_currentDuration.inMinutes}:${(_currentDuration.inSeconds % 60).toString().padLeft(2, '0')}';
    }

    return Scaffold(
      appBar: AppBar(
        title: const Text('FreeTarget'),
        leading: Padding(
          padding: const EdgeInsets.all(8.0),
          child: Icon(
            Icons.wifi,
            color: isConnected ? Colors.green : Colors.red,
          ),
        ),
        actions: [
          IconButton(
            icon: const Icon(Icons.refresh),
            onPressed: _resetScores,
            tooltip: 'Reset Scores',
          ),
          IconButton(
            icon: Icon(showAnalysis ? Icons.area_chart : Icons.show_chart),
            onPressed: () => setState(() => showAnalysis = !showAnalysis),
            tooltip: 'Toggle Group Analysis',
          ),
          IconButton(
            icon: const Icon(Icons.settings),
            onPressed: _showSettingsModal,
            tooltip: 'Hardware Settings',
          ),
          Row(
            children: [
              const Text('Zoom: '),
              SizedBox(
                width: 150,
                child: Slider(
                  value: zoomLevel,
                  min: 1.0,
                  max: 5.0,
                  divisions: 4,
                  label: '${zoomLevel.toStringAsFixed(1)}x',
                  onChanged: (value) => setState(() => zoomLevel = value),
                ),
              ),
            ],
          ),
          SizedBox(
            width: 150,
            child: Padding(
              padding:
                  const EdgeInsets.symmetric(horizontal: 8.0, vertical: 8.0),
              child: TextField(
                controller: ipController,
                decoration: const InputDecoration(
                  isDense: true,
                  filled: true,
                  fillColor: Colors.white,
                  contentPadding:
                      EdgeInsets.symmetric(horizontal: 8, vertical: 8),
                  hintText: 'IP Address',
                ),
                style: const TextStyle(fontSize: 14),
              ),
            ),
          ),
          SizedBox(
            width: 70,
            child: Padding(
              padding:
                  const EdgeInsets.symmetric(horizontal: 8.0, vertical: 8.0),
              child: TextField(
                controller: portController,
                decoration: const InputDecoration(
                  isDense: true,
                  filled: true,
                  fillColor: Colors.white,
                  contentPadding:
                      EdgeInsets.symmetric(horizontal: 8, vertical: 8),
                  hintText: 'Port',
                ),
                style: const TextStyle(fontSize: 14),
              ),
            ),
          ),
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 8.0),
            child: IconButton(
              onPressed: _connect,
              icon: Icon(
                isConnected ? Icons.link_off : Icons.link,
                color: isConnected ? Colors.green : Colors.grey,
              ),
              tooltip: isConnected ? 'Disconnect' : 'Connect',
            ),
          ),
        ],
      ),
      body: Column(
        children: [
          Expanded(
            child: SingleChildScrollView(
              child: Container(
                padding: const EdgeInsets.all(8),
                child: Column(
                  children: [
                    Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: [
                        Row(
                          children: [
                            Row(
                              children: [
                                const Text('Target: ',
                                    style:
                                        TextStyle(fontWeight: FontWeight.bold)),
                                Switch(
                                  value: targetType == TargetType.airRifle,
                                  onChanged: (value) {
                                    setState(() {
                                      targetType = value
                                          ? TargetType.airRifle
                                          : TargetType.airPistol;
                                      for (var series in allSeries) {
                                        series.reset();
                                      }
                                      currentSeriesIndex = 0;
                                      totalScore = 0;
                                      totalDecimalScore = 0;
                                    });
                                  },
                                ),
                                Text(targetType == TargetType.airPistol
                                    ? 'Pistol'
                                    : 'Rifle'),
                              ],
                            ),
                            const SizedBox(width: 20),
                            Row(
                              children: [
                                const Text('Scoring: ',
                                    style:
                                        TextStyle(fontWeight: FontWeight.bold)),
                                Switch(
                                  value: useDecimalScoring,
                                  onChanged: (value) =>
                                      setState(() => useDecimalScoring = value),
                                ),
                                Text(useDecimalScoring ? 'Decimal' : 'Integer'),
                              ],
                            ),
                          ],
                        ),
                        Text(
                          'Total: ${useDecimalScoring ? totalDecimalScore.toStringAsFixed(1) : totalScore.toStringAsFixed(0)}',
                          style: const TextStyle(
                              fontSize: 20, fontWeight: FontWeight.bold),
                        ),
                      ],
                    ),
                    Row(
                      children: [
                        const Text('Timer: ',
                            style: TextStyle(fontWeight: FontWeight.bold)),
                        Switch(
                          value: isTimerEnabled,
                          onChanged: (value) {
                            setState(() {
                              isTimerEnabled = value;
                              if (isTimerEnabled) {
                                _resetTimer();
                              } else {
                                _timer?.cancel();
                              }
                            });
                          },
                        ),
                        if (isTimerEnabled) ...[
                          TextButton(
                            onPressed: _showDurationPicker,
                            child: Text(
                              timeString,
                              style: const TextStyle(
                                fontSize: 18,
                                fontWeight: FontWeight.bold,
                              ),
                            ),
                          ),
                          IconButton(
                            icon: Icon(
                                isTimerPaused ? Icons.play_arrow : Icons.pause),
                            onPressed: _toggleTimer,
                          ),
                          IconButton(
                            icon: const Icon(Icons.refresh),
                            onPressed: _resetTimer,
                          ),
                        ],
                      ],
                    ),
                    GridView.builder(
                      shrinkWrap: true,
                      physics: const NeverScrollableScrollPhysics(),
                      gridDelegate:
                          const SliverGridDelegateWithFixedCrossAxisCount(
                        crossAxisCount: 3,
                        childAspectRatio: 2.5,
                        mainAxisSpacing: 8,
                        crossAxisSpacing: 8,
                      ),
                      itemCount: 6,
                      itemBuilder: (context, seriesIndex) {
                        final series = allSeries[seriesIndex];
                        return Card(
                          color: seriesIndex == currentSeriesIndex
                              ? Colors.blue[50]
                              : null,
                          child: Column(
                            children: [
                              Padding(
                                padding: const EdgeInsets.all(4.0),
                                child: Row(
                                  mainAxisAlignment:
                                      MainAxisAlignment.spaceBetween,
                                  children: [
                                    Text('Series ${seriesIndex + 1}',
                                        style: const TextStyle(
                                            fontWeight: FontWeight.bold)),
                                    Text(
                                      useDecimalScoring
                                          ? series.decimalTotalScore
                                              .toStringAsFixed(1)
                                          : series.totalScore
                                              .toStringAsFixed(0),
                                      style: const TextStyle(
                                          fontWeight: FontWeight.bold),
                                    ),
                                  ],
                                ),
                              ),
                              Expanded(
                                child: GridView.builder(
                                  shrinkWrap: true,
                                  physics: const NeverScrollableScrollPhysics(),
                                  gridDelegate:
                                      const SliverGridDelegateWithFixedCrossAxisCount(
                                    crossAxisCount: 5,
                                    childAspectRatio: 1.5,
                                  ),
                                  itemCount: 10,
                                  itemBuilder: (context, shotIndex) {
                                    if (shotIndex < series.shots.length) {
                                      final shot = series.shots[shotIndex];
                                      return Container(
                                        decoration: BoxDecoration(
                                          border: Border.all(
                                              color: Colors.grey[300]!),
                                        ),
                                        child: Center(
                                          child: Text(
                                            useDecimalScoring
                                                ? '${shot.decimalScore.toStringAsFixed(1)}${shot.isInnerTen ? '*' : ''}'
                                                : shot.score.toStringAsFixed(0),
                                            style: const TextStyle(
                                                fontWeight: FontWeight.bold),
                                          ),
                                        ),
                                      );
                                    }
                                    return Container(
                                      decoration: BoxDecoration(
                                        border: Border.all(
                                            color: Colors.grey[300]!),
                                      ),
                                      child: const Center(child: Text('-')),
                                    );
                                  },
                                ),
                              ),
                            ],
                          ),
                        );
                      },
                    ),
                    const SizedBox(height: 8),
                    SizedBox(
                      height: MediaQuery.of(context).size.width * 0.9,
                      child: Column(
                        children: [
                          Row(
                            mainAxisAlignment: MainAxisAlignment.center,
                            children: [
                              IconButton(
                                icon: const Icon(Icons.arrow_left),
                                onPressed: _canSwipeTargets &&
                                        currentSeriesIndex > 0
                                    ? () {
                                        _targetPageController.previousPage(
                                          duration:
                                              const Duration(milliseconds: 300),
                                          curve: Curves.easeInOut,
                                        );
                                      }
                                    : null,
                              ),
                              Text(
                                'Series ${currentSeriesIndex + 1}',
                                style: const TextStyle(
                                  fontSize: 18,
                                  fontWeight: FontWeight.bold,
                                ),
                              ),
                              IconButton(
                                icon: const Icon(Icons.arrow_right),
                                onPressed: _canSwipeTargets &&
                                        currentSeriesIndex < 5 &&
                                        allSeries[currentSeriesIndex + 1]
                                            .shots
                                            .isNotEmpty
                                    ? () {
                                        _targetPageController.nextPage(
                                          duration:
                                              const Duration(milliseconds: 300),
                                          curve: Curves.easeInOut,
                                        );
                                      }
                                    : null,
                              ),
                            ],
                          ),
                          Expanded(
                            child: Center(
                              child: AspectRatio(
                                aspectRatio: 1,
                                child: PageView.builder(
                                  controller: _targetPageController,
                                  physics: _canSwipeTargets
                                      ? const AlwaysScrollableScrollPhysics()
                                      : const NeverScrollableScrollPhysics(),
                                  onPageChanged: (index) {
                                    setState(() {
                                      currentSeriesIndex = index;
                                    });
                                  },
                                  itemCount: 6,
                                  itemBuilder: (context, index) {
                                    return InteractiveViewer(
                                      scaleEnabled: true,
                                      minScale: 1.0,
                                      maxScale: 5.0,
                                      child: CustomPaint(
                                        size: Size.square(
                                            MediaQuery.of(context).size.width *
                                                0.8),
                                        painter:
                                            targetType == TargetType.airPistol
                                                ? AirPistolTargetPainter(
                                                    allSeries[index].shots,
                                                    zoomLevel,
                                                  )
                                                : AirRifleTargetPainter(
                                                    allSeries[index].shots,
                                                    zoomLevel,
                                                  ),
                                        foregroundPainter: showAnalysis
                                            ? GroupAnalysisPainter(
                                                shots: allSeries[index].shots,
                                                scale: (MediaQuery.of(context)
                                                            .size
                                                            .width *
                                                        0.8) /
                                                    (targetType ==
                                                            TargetType.airPistol
                                                        ? (170.0 * 1.2)
                                                        : (80.0 * 1.2)),
                                              )
                                            : null,
                                      ),
                                    );
                                  },
                                ),
                              ),
                            ),
                          ),
                        ],
                      ),
                    ),
                  ],
                ),
              ),
            ),
          ),
          Container(
            color: Colors.grey[200],
            child: Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                GestureDetector(
                  onTap: () =>
                      setState(() => isStatusExpanded = !isStatusExpanded),
                  child: Padding(
                    padding: const EdgeInsets.symmetric(
                        horizontal: 8.0, vertical: 4.0),
                    child: Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: [
                        Row(
                          children: [
                            const Text('Status: ',
                                style: TextStyle(fontWeight: FontWeight.bold)),
                            Text(isConnected ? 'Connected' : 'Disconnected'),
                          ],
                        ),
                        Icon(isStatusExpanded
                            ? Icons.expand_less
                            : Icons.expand_more),
                      ],
                    ),
                  ),
                ),
                if (isStatusExpanded)
                  Container(
                    constraints: BoxConstraints(
                      maxHeight: MediaQuery.of(context).size.height * 0.3,
                    ),
                    child: SingleChildScrollView(
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Padding(
                            padding: const EdgeInsets.all(8.0),
                            child: Row(
                              children: [
                                const Text('Last Message: ',
                                    style:
                                        TextStyle(fontWeight: FontWeight.bold)),
                                Expanded(
                                  child: Text(lastMessage,
                                      overflow: TextOverflow.ellipsis),
                                ),
                              ],
                            ),
                          ),
                          const Divider(height: 1),
                          ListView.builder(
                            shrinkWrap: true,
                            physics: const NeverScrollableScrollPhysics(),
                            itemCount: recentMessages.length,
                            itemBuilder: (context, index) => Padding(
                              padding: const EdgeInsets.symmetric(
                                horizontal: 8.0,
                                vertical: 2.0,
                              ),
                              child: Text(recentMessages[index]),
                            ),
                          ),
                        ],
                      ),
                    ),
                  ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  @override
  void dispose() {
    _targetPageController.dispose();
    _timer?.cancel();
    connectionHandler.dispose();
    ipController.dispose();
    portController.dispose();
    super.dispose();
  }
}
