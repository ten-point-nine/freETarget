import 'package:flutter/material.dart';
import '../models/series.dart';
import '../models/shot.dart';
import '../painters/target_painters.dart';

class SessionReviewScreen extends StatelessWidget {
  final List<Series> allSeries;
  final TargetType targetType;
  final bool useDecimalScoring;
  final double totalScore;
  final double totalDecimalScore;
  final DateTime sessionStartTime;

  const SessionReviewScreen({
    super.key,
    required this.allSeries,
    required this.targetType,
    required this.useDecimalScoring,
    required this.totalScore,
    required this.totalDecimalScore,
    required this.sessionStartTime,
  });

  String _formatDate(DateTime dateTime) {
    return '${dateTime.day}/${dateTime.month}/${dateTime.year}';
  }

  String _formatTime(DateTime dateTime) {
    return '${dateTime.hour.toString().padLeft(2, '0')}:${dateTime.minute.toString().padLeft(2, '0')}';
  }

  @override
  Widget build(BuildContext context) {
    final screenWidth = MediaQuery.of(context).size.width;
    
    return Scaffold(
      appBar: AppBar(
        title: const Text('Session Review'),
        backgroundColor: const Color(0xFF1E3A8A),
        foregroundColor: Colors.white,
        actions: [
          IconButton(
            icon: const Icon(Icons.save_alt),
            onPressed: () {
              ScaffoldMessenger.of(context).showSnackBar(
                const SnackBar(
                  content: Text('Please take a screenshot to save this report'),
                  duration: Duration(seconds: 3),
                ),
              );
            },
            tooltip: 'Save Report',
          ),
        ],
      ),
      body: SingleChildScrollView(
        child: Column(
          children: [
            // 4x2 Grid layout
            GridView.count(
              shrinkWrap: true,
              physics: const NeverScrollableScrollPhysics(),
              crossAxisCount: 2,
              childAspectRatio: 0.85,
              padding: const EdgeInsets.all(8),
              mainAxisSpacing: 8,
              crossAxisSpacing: 8,
              children: [
                // First block: App Info and Event Type
                Card(
                  color: const Color(0xFF1E3A8A),
                  child: Padding(
                    padding: const EdgeInsets.all(16),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        const Text(
                          'Freetarget',
                          style: TextStyle(
                            fontSize: 24,
                            fontWeight: FontWeight.bold,
                            color: Colors.white,
                          ),
                        ),
                        const SizedBox(height: 8),
                        Text(
                          targetType == TargetType.airPistol
                              ? 'ISSF 10m Air Pistol'
                              : 'ISSF 10m Air Rifle',
                          style: const TextStyle(
                            fontSize: 18,
                            color: Colors.blue,
                          ),
                        ),
                        const Divider(color: Colors.white24),
                        const Spacer(),
                        _buildDetailRow('Date', _formatDate(sessionStartTime)),
                        _buildDetailRow('Time', _formatTime(sessionStartTime)),
                      ],
                    ),
                  ),
                ),

                // Second block: Overall Score and Combined Target
                Card(
                  child: Padding(
                    padding: const EdgeInsets.all(8),
                    child: Column(
                      children: [
                        Text(
                          'Total Score: ${useDecimalScoring ? totalDecimalScore.toStringAsFixed(1) : totalScore.toStringAsFixed(0)}',
                          style: const TextStyle(
                            fontSize: 20,
                            fontWeight: FontWeight.bold,
                          ),
                        ),
                        const SizedBox(height: 8),
                        Expanded(
                          child: InteractiveViewer(
                            scaleEnabled: true,
                            minScale: 1.0,
                            maxScale: 5.0,
                            child: CustomPaint(
                              size: Size.square(screenWidth * 0.4),
                              painter: targetType == TargetType.airPistol
                                  ? AirPistolTargetPainter(
                                      allSeries.expand((s) => s.shots).toList(),
                                      1.0,
                                    )
                                  : AirRifleTargetPainter(
                                      allSeries.expand((s) => s.shots).toList(),
                                      1.0,
                                    ),
                            ),
                          ),
                        ),
                      ],
                    ),
                  ),
                ),

                // Blocks 3-8: Individual Series
                for (var i = 0; i < 6; i++)
                  Card(
                    child: Padding(
                      padding: const EdgeInsets.all(8),
                      child: Column(
                        children: [
                          Row(
                            mainAxisAlignment: MainAxisAlignment.spaceBetween,
                            children: [
                              Text(
                                'Series ${i + 1}',
                                style: const TextStyle(
                                  fontSize: 16,
                                  fontWeight: FontWeight.bold,
                                ),
                              ),
                              Text(
                                useDecimalScoring
                                    ? allSeries[i].decimalTotalScore.toStringAsFixed(1)
                                    : allSeries[i].totalScore.toStringAsFixed(0),
                                style: const TextStyle(
                                  fontSize: 16,
                                  fontWeight: FontWeight.bold,
                                ),
                              ),
                            ],
                          ),
                          const SizedBox(height: 8),
                          Expanded(
                            child: InteractiveViewer(
                              scaleEnabled: true,
                              minScale: 1.0,
                              maxScale: 5.0,
                              child: CustomPaint(
                                size: Size.square(screenWidth * 0.35),
                                painter: targetType == TargetType.airPistol
                                    ? AirPistolTargetPainter(
                                        allSeries[i].shots,
                                        1.0,
                                      )
                                    : AirRifleTargetPainter(
                                        allSeries[i].shots,
                                        1.0,
                                      ),
                              ),
                            ),
                          ),
                          const SizedBox(height: 8),
                          // Shot scores row
                          SizedBox(
                            height: 30,
                            child: ListView.builder(
                              scrollDirection: Axis.horizontal,
                              itemCount: allSeries[i].shots.length,
                              itemBuilder: (context, shotIndex) {
                                final shot = allSeries[i].shots[shotIndex];
                                return Container(
                                  width: 35,
                                  alignment: Alignment.center,
                                  margin: const EdgeInsets.symmetric(horizontal: 2),
                                  decoration: BoxDecoration(
                                    border: Border.all(color: Colors.grey),
                                    borderRadius: BorderRadius.circular(4),
                                  ),
                                  child: Text(
                                    useDecimalScoring
                                        ? '${shot.decimalScore.toStringAsFixed(1)}${shot.isInnerTen ? '*' : ''}'
                                        : shot.score.toStringAsFixed(0),
                                    style: const TextStyle(
                                      fontSize: 12,
                                      fontWeight: FontWeight.bold,
                                    ),
                                  ),
                                );
                              },
                            ),
                          ),
                        ],
                      ),
                    ),
                  ),
              ],
            ),
            const Padding(
              padding: EdgeInsets.all(16.0),
              child: Text(
                'Take a screenshot to save this report',
                style: TextStyle(
                  color: Colors.grey,
                  fontStyle: FontStyle.italic,
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildDetailRow(String label, String value) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 2),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(
            label,
            style: const TextStyle(
              fontSize: 12,
              color: Colors.blue,
            ),
          ),
          Text(
            value,
            style: const TextStyle(
              fontSize: 12,
              color: Colors.white,
            ),
          ),
        ],
      ),
    );
  }
}