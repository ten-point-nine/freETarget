import 'shot.dart';

class Series {
  List<Shot> shots = [];
  double totalScore = 0;
  double decimalTotalScore = 0;

  void reset() {
    shots.clear();
    totalScore = 0;
    decimalTotalScore = 0;
  }
}