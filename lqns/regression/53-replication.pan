<?xml version="1.0"?>
<!-- lqns -pragma=variance=mol,threads=hyper -no-warnings -pragma=replication=pan -xml -output=53-replication.pan -->
<lqn-model name="53-replication" description="lqns 5.32 solution for 53-replication.lqnx: $FIAB = 4, $FIBC = 2, $FOAB = 2, $FOBC = 2, $KA = 4, $KB = 2, $KC = 2." xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="/usr/local/share/lqns/lqn.xsd">
  <solver-params comment="Pan Thesis, Case-4." conv_val="1e-05" it_limit="500" underrelax_coeff="0.9" print_int="10">
    <pragma param="replication" value="pan"/>
    <pragma param="severity-level" value="run-time"/>
    <pragma param="threads" value="hyper"/>
    <pragma param="variance" value="mol"/>
    <result-general solver-info="lqns 5.32" valid="true" conv-val="8.65043e-06" iterations="4" platform-info="Mac Darwin 24.6.0" user-cpu-time=" 0:00:00.001" system-cpu-time=" 0:00:00.000" elapsed-time=" 0:00:00.001" max-rss="7192576">
      <mva-info submodels="3" core="47" step="589" step-squared="9121" wait="54194" wait-squared="1.3009e+08" faults="0"/>
    </result-general>
  </solver-params>
  <processor name="A1" scheduling="fcfs" replication="4">
    <result-processor utilization="0.0573651"/>
    <task name="A1" scheduling="ref" replication="4">
      <result-task throughput="0.0573651" utilization="0.999998" phase1-utilization="0.999998" proc-utilization="0.0573651"/>
      <fan-out dest="B1" value="2"/>
      <entry name="A1" type="PH1PH2">
        <result-entry utilization="0.999998" throughput="0.0573651" proc-utilization="0.0573651" squared-coeff-variation="2.30026" throughput-bound="0.142857"/>
        <entry-phase-activities>
          <activity name="A1_1" phase="1" host-demand-mean="1">
            <result-activity proc-waiting="0" service-time="17.4322" utilization="0.999998" service-time-variance="699.006"/>
            <synch-call dest="B1" calls-mean="1">
              <result-call waiting="4.73006"/>
            </synch-call>
          </activity>
        </entry-phase-activities>
      </entry>
    </task>
  </processor>
  <processor name="B1" scheduling="fcfs" replication="2">
    <result-processor utilization="0.229462"/>
    <task name="B1" scheduling="fcfs" replication="2">
      <result-task throughput="0.229462" utilization="0.799911" phase1-utilization="0.799911" proc-utilization="0.229462"/>
      <fan-in source="A1" value="4"/>
      <fan-out dest="C1" value="2"/>
      <entry name="B1" type="PH1PH2">
        <result-entry utilization="0.799911" throughput="0.229462" proc-utilization="0.229462" squared-coeff-variation="1.59333" throughput-bound="0.333333"/>
        <entry-phase-activities>
          <activity name="B1_1" phase="1" host-demand-mean="1">
            <result-activity proc-waiting="0" service-time="3.48603" utilization="0.799911" service-time-variance="19.3628"/>
            <synch-call dest="C1" calls-mean="1">
              <result-call waiting="0.243014"/>
            </synch-call>
          </activity>
        </entry-phase-activities>
      </entry>
    </task>
  </processor>
  <processor name="C1" scheduling="fcfs" replication="2">
    <result-processor utilization="0.458926"/>
    <task name="C1" scheduling="fcfs" replication="2">
      <result-task throughput="0.458926" utilization="0.458926" phase1-utilization="0.458926" proc-utilization="0.458926"/>
      <fan-in source="B1" value="2"/>
      <entry name="C1" type="PH1PH2">
        <result-entry utilization="0.458926" throughput="0.458926" proc-utilization="0.458926" squared-coeff-variation="1" throughput-bound="1"/>
        <entry-phase-activities>
          <activity name="C1_1" phase="1" host-demand-mean="1">
            <result-activity proc-waiting="0" service-time="1" utilization="0.458926" service-time-variance="1"/>
          </activity>
        </entry-phase-activities>
      </entry>
    </task>
  </processor>
</lqn-model>
